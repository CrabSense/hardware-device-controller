#!/usr/bin/env python3
"""Reference bridge; one process per provisioned device. Firmware ON remains locked."""
import datetime as dt
import hashlib
import hmac
import ipaddress
import json
import os
import time
import urllib.parse
import urllib.request
import uuid

class NoRedirect(urllib.request.HTTPRedirectHandler):
    def redirect_request(self, *args, **kwargs):
        return None

OPENER = urllib.request.build_opener(NoRedirect())

def sign(key, raw):
    return hmac.new(key.encode('ascii'), raw, hashlib.sha256).hexdigest()

def epoch(value):
    d = dt.datetime.fromisoformat(value.replace('Z', '+00:00'))
    if d.tzinfo is None: raise ValueError('timezone required')
    return int(d.timestamp())

def channel(value, command):
    if command == 'alloff':
        if value not in (None, ''): raise ValueError('alloff channel must be null')
        return 0
    aliases = {str(i): i for i in range(1,33)}
    for prefix in ('ssr','pump','pump_0','pump_'):
        aliases.update({f'{prefix}{i}':i for i in range(1,33)})
    return aliases[str(value).lower().strip()]

def command_envelope(c, device, now):
    if c['deviceCode'] != device or c['status'] != 'delivered': raise ValueError('wrong device/status')
    cmd=c['command']; seq=c['commandSequence']
    if cmd not in ('on','off','alloff') or type(seq) is not int or not 0<seq<2**63: raise ValueError('invalid sequence/command')
    for k in ('id','deliveryAttemptId'): uuid.UUID(c[k])
    deadline=min(epoch(c['expiresAt']),epoch(c['leaseExpiresAt']),now+10)
    if deadline<=now: raise ValueError('expired lease')
    return dict(protocol=2,kind='command',deviceCode=device,nonce=str(uuid.uuid4()),id=c['id'],deliveryAttemptId=c['deliveryAttemptId'],commandSequence=seq,command=cmd,channel=channel(c['channel'],cmd),expiresEpoch=deadline)

def request(url, headers, data=None):
    req=urllib.request.Request(url,data=data,headers=headers,method='GET' if data is None else 'POST')
    with OPENER.open(req, timeout=3) as resp:
        raw=resp.read(262145)
        if len(raw)>262144: raise ValueError('response too large')
        return raw,resp.headers

class Bridge:
    def __init__(self, cloud, local, device, device_key, local_key, channels=None):
        u=urllib.parse.urlsplit(cloud)
        if u.scheme!='https' or not u.hostname or u.username or u.password or u.query or u.fragment: raise ValueError('HTTPS cloud required')
        l=urllib.parse.urlsplit(local)
        if l.scheme!='http' or l.username or l.password or l.path not in ('','/') or l.query or l.fragment: raise ValueError('local HTTP IP required')
        ip=ipaddress.ip_address(l.hostname)
        if not ip.is_private or ip.is_loopback or ip.is_unspecified or ip.is_link_local: raise ValueError('isolated LAN IP required')
        if len(local_key)!=64 or any(c not in '0123456789abcdef' for c in local_key) or not device_key: raise ValueError('invalid keys')
        self.channels=frozenset()
        self.reported_channels=frozenset()
        self.control_disabled=True
        self.manifest_version=None
        # Legacy argument is accepted for callers but never trusted as capability.
        self.cloud=cloud.rstrip('/'); self.local=local.rstrip('/'); self.device=device; self.key=local_key
        self.headers={'X-Device-Key':device_key,'X-Edge-Protocol':'2','Content-Type':'application/json'}
    def cloud_call(self,path,data=None):
        raw,_=request(self.cloud+path,self.headers,None if data is None else json.dumps(data).encode())
        out=json.loads(raw)
        if out.get('success') is not True: raise ValueError('cloud rejected request')
        return out.get('data')
    def local_call(self,env):
        raw=json.dumps(env,separators=(',',':')).encode()
        response,h=request(self.local+'/api/command',{'Content-Type':'application/json','X-Kiosk-Signature':sign(self.key,raw)},raw)
        if not hmac.compare_digest(h.get('X-Kiosk-Signature',''),sign(self.key,response)): raise ValueError('invalid response signature')
        out=json.loads(response)
        for k in ('nonce','deviceCode','protocol'):
            if out.get(k)!=env[k]: raise ValueError('response mismatch')
        if env['kind']=='command':
            for k in ('id','deliveryAttemptId','commandSequence'):
                if out.get(k)!=env[k]: raise ValueError('command response mismatch')
        if type(out.get('success')) is not bool: raise ValueError('invalid result')
        return out
    def report(self,out):
        states=out.get('outputs')
        if not isinstance(states,list) or len(states)!=len(self.reported_channels): raise ValueError('missing relay observations')
        if {s.get('channel') for s in states}!=self.reported_channels or any(type(s.get('on')) is not bool for s in states): raise ValueError('invalid observations')
        for s in states:
            if s['channel'] in self.channels: self.cloud_call('/api/iot/relay-state',dict(deviceCode=self.device,channel=s['channel'],on=s['on']))
    def discover(self,hb):
        manifest=hb.get('capabilities')
        if not isinstance(manifest,dict) or manifest.get('deviceCode')!=self.device: raise ValueError('missing signed capability manifest')
        ports=manifest.get('ports')
        if not isinstance(ports,list) or not 1<=len(ports)<=64: raise ValueError('invalid capability ports')
        outputs=[p.get('channel') for p in ports if p.get('type')=='DIGITAL_OUTPUT']
        if not outputs or any(type(c) is not int or not 1<=c<=32 for c in outputs) or len(set(outputs))!=len(outputs): raise ValueError('invalid output capability')
        reported=frozenset(outputs)
        states=hb.get('outputs')
        if not isinstance(states,list) or len(states)!=len(reported) or {s.get('channel') for s in states}!=reported:
            raise ValueError('capability/state mismatch')
        result=self.cloud_call('/api/iot/controllers/capabilities',manifest)
        if not isinstance(result,dict) or result.get('configVersion')!=manifest.get('configVersion'): raise ValueError('manifest not accepted')
        bound=result.get('boundOutputChannels')
        if not isinstance(bound,list) or any(type(c) is not int for c in bound) or not set(bound)<=reported: raise ValueError('invalid binding')
        self.reported_channels=reported
        self.channels=frozenset(bound)
        self.control_disabled=result.get('hardwareControlDisabled') is not False
        self.manifest_version=manifest['configVersion']
    def cycle(self):
        hb=self.local_call(dict(protocol=2,kind='heartbeat',deviceCode=self.device,nonce=str(uuid.uuid4()),expiresEpoch=int(time.time())+5))
        if not hb['success']: raise ValueError('heartbeat rejected')
        self.discover(hb)
        self.report(hb)
        if self.control_disabled: return
        commands=self.cloud_call('/api/edge/commands/pending?'+urllib.parse.urlencode({'deviceCode':self.device}))
        if not isinstance(commands,list): raise ValueError('invalid pending response')
        for c in sorted(commands,key=lambda x:x['commandSequence']):
            env=command_envelope(c,self.device,int(time.time()))
            if env['channel'] and env['channel'] not in self.channels: raise ValueError('unconfigured channel')
            out=self.local_call(env)
            self.cloud_call('/api/edge/commands/'+env['id']+'/ack',dict(status='acknowledged' if out['success'] else 'failed',deliveryAttemptId=env['deliveryAttemptId'],message=out['message']))
            self.report(out)

def main():
    bridge=Bridge(*(os.environ[k] for k in ('CRAB_CLOUD_URL','CRAB_ESP_URL','CRAB_DEVICE_CODE','CRAB_DEVICE_KEY','CRAB_LOCAL_KEY')))
    while True:
        try: bridge.cycle()
        except Exception as exc: print('Edge cycle failed:',type(exc).__name__,flush=True)
        time.sleep(2)
if __name__=='__main__': main()
