import unittest, sys, pathlib, uuid, datetime as dt, json, time
from unittest.mock import patch
sys.path.insert(0,str(pathlib.Path(__file__).resolve().parents[1]/'kiosk'))
from bridge import Bridge, command_envelope, sign, channel

def command(**kw):
    now=dt.datetime.now(dt.timezone.utc)
    data=dict(deviceCode='ESP-A',status='delivered',command='off',channel='1',commandSequence=2**53+9,inventoryRevision=2,id=str(uuid.uuid4()),deliveryAttemptId=str(uuid.uuid4()),expiresAt=(now+dt.timedelta(seconds=30)).isoformat(),leaseExpiresAt=(now+dt.timedelta(seconds=15)).isoformat());data.update(kw);return data
class Tests(unittest.TestCase):
    def test_sequence_exact(self):
        c=command();e=command_envelope(c,'ESP-A',int(time.time()));self.assertEqual(e['commandSequence'],c['commandSequence']);self.assertEqual(e['deliveryAttemptId'],c['deliveryAttemptId'])
    def test_expiry(self):
        with self.assertRaises(ValueError):command_envelope(command(expiresAt='2020-01-01T00:00:00Z'),'ESP-A',int(time.time()))
    def test_wrong_device(self):
        with self.assertRaises(ValueError):command_envelope(command(),'ESP-B',int(time.time()))
    def test_toggle(self):
        with self.assertRaises(ValueError):command_envelope(command(command='toggle'),'ESP-A',int(time.time()))
    def test_channel(self):
        self.assertEqual(channel('pump_04','off'),4)
        with self.assertRaises(KeyError):channel('1garbage','on')
    def test_cloud_requires_tls(self):
        with self.assertRaises(ValueError):Bridge('http://example.com','http://192.168.1.4','ESP-A','key','a'*64)
    def test_response_auth(self):
        b=Bridge('https://example.com','http://192.168.1.4','ESP-A','key','a'*64);e=command_envelope(command(),'ESP-A',int(time.time()));raw=json.dumps(dict(e,success=True)).encode()
        with patch('bridge.request',return_value=(raw,{'X-Kiosk-Signature':'bad'})):
            with self.assertRaises(ValueError):b.local_call(e)
        with patch('bridge.request',return_value=(raw,{'X-Kiosk-Signature':sign(b.key,raw)})):
            self.assertTrue(b.local_call(e)['success'])
    def test_response_wrong_attempt(self):
        b=Bridge('https://example.com','http://192.168.1.4','ESP-A','key','a'*64);e=command_envelope(command(),'ESP-A',int(time.time()));raw=json.dumps(dict(e,success=True,deliveryAttemptId=str(uuid.uuid4()))).encode()
        with patch('bridge.request',return_value=(raw,{'X-Kiosk-Signature':sign(b.key,raw)})):
            with self.assertRaises(ValueError):b.local_call(e)
    def manifest(self,n=5):
        return dict(deviceCode='ESP-A',hardwareModel='ESP32-V1',firmwareVersion='2.1',configVersion=2,
            ports=[dict(portCode=f'OUT-{i:02}',type='DIGITAL_OUTPUT',channel=i,gpio=i+3,activeLevel='HIGH',on=False) for i in range(1,n+1)])
    def heartbeat(self,n=5):
        return dict(success=True,commandStorageReady=True,outputs=[dict(channel=i,on=False) for i in range(1,n+1)],capabilities=self.manifest(n))
    def test_auto_discovery_channel_five_while_control_locked(self):
        b=Bridge('https://example.com','http://192.168.1.4','ESP-A','key','a'*64)
        with patch.object(b,'local_call',return_value=self.heartbeat()),patch.object(b,'cloud_call',side_effect=[dict(configVersion=2,boundOutputChannels=[],hardwareControlDisabled=True)]) as cloud:
            b.cycle()
            self.assertEqual(b.reported_channels,frozenset(range(1,6)))
            self.assertEqual(cloud.call_count,1)
    def test_no_ack_on_transport_failure(self):
        b=Bridge('https://example.com','http://192.168.1.4','ESP-A','key','a'*64)
        with patch.object(b,'cloud_call',return_value=[command()]) as cloud,patch.object(b,'local_call',side_effect=TimeoutError):
            with self.assertRaises(TimeoutError):b.cycle()
            self.assertEqual(cloud.call_count,0)
    def test_report_only_registered_channels(self):
        b=Bridge('https://example.com','http://192.168.1.4','ESP-A','key','a'*64,channels=(1,2))
        b.channels=frozenset((1,2)); b.reported_channels=frozenset(range(1,5))
        with patch.object(b,'cloud_call') as cloud:
            b.report({'outputs':[{'channel':i,'on':False} for i in range(1,5)]})
            self.assertEqual([call.args[1]['channel'] for call in cloud.call_args_list],[1,2])
    def test_ack_uses_current_attempt(self):
        b=Bridge('https://example.com','http://192.168.1.4','ESP-A','key','a'*64,channels=(1,))
        c=command(); states=[{'channel':i,'on':False} for i in range(1,5)]
        with patch.object(b,'cloud_call',side_effect=[dict(configVersion=2,boundOutputChannels=[1],hardwareControlDisabled=False),None,[c],None,None]) as cloud, patch.object(b,'local_call',side_effect=[self.heartbeat(4),{'success':True,'message':'applied','outputs':states}]):
            b.cycle()
            ack=cloud.call_args_list[3];self.assertEqual(ack.args[0],'/api/edge/commands/'+c['id']+'/ack');self.assertEqual(ack.args[1]['deliveryAttemptId'],c['deliveryAttemptId'])
if __name__=='__main__': unittest.main()
