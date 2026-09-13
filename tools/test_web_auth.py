"""Read-only regression: two browser challenges must not invalidate each other."""
import hashlib,json,re,secrets,urllib.request,urllib.error
import argparse
from pathlib import Path
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--ip',required=True)
parser.add_argument('--access',type=Path,default=Path(__file__).resolve().parents[1]/'private/access.json')
args=parser.parse_args()
if not re.fullmatch(r'[a-zA-Z0-9.-]+',args.ip):parser.error('Expected an IP or hostname')
access=json.loads(args.access.read_text(encoding='utf-8-sig')) if args.access.exists() else {}
USERNAME=access.get('web_username','admin');PASSWORD=access.get('web_password','admin')
BASE='http://'+args.ip
http=urllib.request.build_opener(urllib.request.ProxyHandler({}))
def request(path,auth=None):
 q=urllib.request.Request(BASE+path,headers={'Authorization':auth} if auth else {})
 try:r=http.open(q,timeout=10)
 except urllib.error.HTTPError as e:r=e
 with r:return r.status,r.headers,r.read()
def challenge():
 status,h,_=request('/settings');assert status==401
 return dict(re.findall(r'(\w+)="([^"]*)"',h['WWW-Authenticate']))
def authorization(c,path,password=PASSWORD,nc=1):
 md=lambda s:hashlib.md5(s.encode()).hexdigest()
 cn=secrets.token_hex(8);count=f'{nc:08x}'
 response=md(md(USERNAME+':'+c['realm']+':'+password)+':'+c['nonce']+':'+count+':'+cn+':auth:'+md('GET:'+path))
 return 'Digest '+', '.join(f'{k}="{v}"' for k,v in {'username':USERNAME,'realm':c['realm'],'nonce':c['nonce'],'uri':path,'response':response,'opaque':c['opaque'],'cnonce':cn}.items())+', qop=auth, nc='+count
a=challenge();b=challenge()
status,_,_=request('/settings/config',authorization(a,'/settings/config'))
print('First browser after second challenge:',status,flush=True)
assert status==200,'Second challenge invalidated the first browser'
for i in range(12):
 challenge()
 for c,path in [(a,'/settings/config'),(b,'/mqtt/status'),(a,'/settings?test=1')]:
  assert request(path,authorization(c,path,nc=i+2))[0]==200
assert request('/settings',authorization(a,'/settings',PASSWORD+'-wrong'))[0]==401
assert request('/settings',authorization(a,'/mqtt'))[0]==401
old={**a,'nonce':'0'*32}
status,h,_=request('/settings',authorization(old,'/settings'))
assert status==401 and 'stale=true' in h['WWW-Authenticate']
print('PASS interleaved browsers, wrong password, wrong URI and stale nonce recovery',flush=True)
