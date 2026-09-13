"""Generate the HTTPS trust roots used by GitHub and its CDN from Mozilla's CA set."""
from pathlib import Path
import urllib.request,hashlib,re
from cryptography import x509
from cryptography.x509.oid import NameOID

r=Path(__file__).resolve().parents[1]
source='https://curl.se/ca/cacert.pem'
data=urllib.request.urlopen(source,timeout=30).read()
names={'ISRG Root X1','ISRG Root X2','USERTrust ECC Certification Authority','Sectigo Public Server Authentication Root E46'}
selected=[]
for pem in re.findall(rb'-----BEGIN CERTIFICATE-----.*?-----END CERTIFICATE-----',data,re.S):
    cert=x509.load_pem_x509_certificate(pem)
    cn=cert.subject.get_attributes_for_oid(NameOID.COMMON_NAME)
    if cn and cn[0].value in names:selected.append((cn[0].value,pem.decode()))
assert {'ISRG Root X1','ISRG Root X2'}.issubset({n for n,_ in selected})
text='// Public trust anchors from Mozilla via '+source+'\n// Source SHA256: '+hashlib.sha256(data).hexdigest()+'\n#pragma once\nstatic const char UPDATE_CA_CERTS[] = R"CA(\n'+'\n'.join(p for _,p in selected)+'\n)CA";\n'
(r/'firmware/rack_bootstrap/UpdateCertificates.h').write_text(text,encoding='utf-8')
print('Selected verified public roots:',', '.join(n for n,_ in selected))
