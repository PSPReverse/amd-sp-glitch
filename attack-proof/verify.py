#!/usr/bin/env python
# Copyright (C) 2021 Niklas Jacob, Robert Buhren
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

import os, sys, struct, binascii, datetime
from random import randint
from cryptography import x509
from cryptography.x509 import NameOID
from cryptography.hazmat.primitives.asymmetric import ec
from cryptography.hazmat.primitives.asymmetric import rsa
from cryptography.hazmat.primitives.asymmetric import padding
from cryptography.hazmat.primitives.asymmetric import utils
from cryptography.hazmat.primitives.serialization import Encoding, PublicFormat, NoEncryption, PrivateFormat, load_pem_private_key
from cryptography.exceptions import InvalidSignature

def read_file_bytes(filename):
    with open(filename, "rb") as file:
        return bytes(file.read())

def verify_amd_rsa_sig(public_key, data, signature):
    if public_key.key_size == 2048:
        hash_algo = utils.hashes.SHA256
        salt_length = 32
    elif public_key.key_size == 4096:
        hash_algo = utils.hashes.SHA384
        salt_length = 48
    else:
        raise NotImplementedError("Only 2048 bit or 4096 bit RSA key checking is implemented!")

    try:
        public_key.verify(
            bytes(signature),
            bytes(data),
            padding.PSS(
                mgf=padding.MGF1(hash_algo()),
                salt_length=salt_length
            ),
            hash_algo()
        )
    except InvalidSignature:
        return False
    return True

def verify_ecdsa_sha384_sig(public_key, data, signature):
    try:
        public_key.verify(
            bytes(signature),
            bytes(data),
            ec.ECDSA(utils.hashes.SHA384()),
        )
    except InvalidSignature:
        return False
    return True

def sha384_digest(data):
    h = utils.hashes.Hash(utils.hashes.SHA384())
    h.update(data)
    return h.finalize()

def file_exists_or_download(filename, download_url):
    print()
    if not os.path.exists(filename):
        print(f"### Downloading {filename}")
        os.system(f'wget -O {filename} "{download_url}"')
    else:
        print(f"### Reusing {filename}")

class AmdRsaCert:
    def __init__(self, buf):
        self.buf = bytes(buf)

        assert len(self.buf) >= 0x40, "Buffer must be large enough to contain header!"

        self.version = struct.unpack("<I", self.buf[0x00:0x04])[0]
        assert self.version == 1

        self.key_id = self.buf[0x04:0x14]

        self.cert_id = self.buf[0x14:0x24]

        self.key_usage = struct.unpack("<I", self.buf[0x24:0x28])[0]
        assert self.key_usage in {0, 0x13}, "KEY_USAGE must be either 'AMD Root Key' (0) or 'AMD SEV Signing Key' (0x13)"
        self.is_ark = self.key_usage == 0
        if self.is_ark:
            assert self.key_id == self.cert_id, "AMD Root Key needs to be 'self-signed'"

        self.pubexp_size = struct.unpack("<I", self.buf[0x38:0x3c])[0]
        assert self.pubexp_size in {2048, 4096}, "PUBEXP_SIZE must be 2048 or 4096"
        self.pubexp_size_bytes = self.pubexp_size >> 3

        self.modulus_size = struct.unpack("<I", self.buf[0x3c:0x40])[0]
        assert self.modulus_size in {2048, 4096}, "MODULUS_SIZE must be 2048 or 4096"
        self.modulus_size_bytes = self.modulus_size >> 3

        self.pubexp_start = 0x40
        self.modulus_start = self.pubexp_start + self.pubexp_size_bytes
        self.signature_start = self.modulus_start + self.modulus_size_bytes

        assert len(self.buf) >= self.signature_start, "Buffer needs to be large enough for header and public key!"

        self.pubexp_bytes = self.buf[self.pubexp_start:self.modulus_start]
        self.pubexp = int.from_bytes(self.pubexp_bytes, 'little')
        assert self.pubexp == 0x10001, "The public exponent is always 0x10001!"

        self.modulus_bytes = self.buf[self.modulus_start:self.signature_start]
        self.modulus = int.from_bytes(self.modulus_bytes, 'little')

        self.public_key = rsa.RSAPublicNumbers(self.pubexp, self.modulus).public_key()

        if len(self.buf) < self.signature_start + self.modulus_size_bytes:
            self.has_signature = False
            self.size = self.signature_start
            del self.signature_start
        else:
            self.has_signature = True
            self.size = self.signature_start + self.modulus_size_bytes

            self.signed_bytes = self.buf[:self.signature_start]
            self.signature_bytes = int.from_bytes(self.buf[self.signature_start:self.size], 'little').to_bytes(self.modulus_size_bytes, 'big')

        self.buf = self.buf[:self.size]

    def check_amd_cert(self, cert):
        assert cert.has_signature
        assert self.key_id == cert.cert_id, f"Certificate must be signed by {self.key_id}"
        assert self.modulus_size == cert.modulus_size, f"Certificates must have the same modulus size!"

        return check_cert(self, cert)

    def check_cert(self, cert):
        return verify_amd_rsa_sig(
            self.public_key,
            cert.signed_bytes,
            cert.signature_bytes,
        )

    def write_pubkey_as_pem(self, filename):
        with open(filename, "wb") as file:
            file.write(self.public_key.public_bytes(
                Encoding.PEM,
                PublicFormat.SubjectPublicKeyInfo,
            ))

class AmdAskArkCert:

    ASK_ARK_HASHES = {
        '41ed65c78aa2a42a70bbdda05ecd3c4f5a2c34eb07a79359600c6afb188b3b6b2235b3fc9d283cec38307596d0e68ea6' : 'Milan',
        '7cc74c72fd2468c149f77fba5bcd8c5910cc4e06e1ac1b12d55afd549683dcf01681e6f2071b62311d71f78e38db0e2a' : 'Rome',
        '3e1078f7ac88e2852235a655bccf061a66e12a64e1c9bc194d6a76bf1ad969ede4a877da4e97ab4ee372736a7cbffa48' : 'Naples',
    }

    def __init__(self, filename):

        with open(filename, "rb") as file:
            self.file_bytes = bytes(file.read())

        self.ask = AmdRsaCert(self.file_bytes)
        assert not self.ask.is_ark

        self.ark = AmdRsaCert(self.file_bytes[self.ask.size:])
        assert self.ark.is_ark

        self.µ_arch = "Unknown"
        self.file_hash_known = False
        try:
            self.µ_arch = self.ASK_ARK_HASHES[binascii.hexlify(sha384_digest(self.file_bytes)).decode()]
            self.file_hash_known = True
        except KeyError:
            pass

        if self.ark.has_signature:
            self.ark_valid = self.ark.check_cert(self.ark)
        else:
            self.ark_valid = "<not signed>"
        self.ask_valid = self.ark.check_cert(self.ask)

    def print_verification(self):

        errors = ''
    
        print(f"")
        print(f"#### ARK (Amd Root Signing Key)")
        print(f"  key_id = {binascii.hexlify(self.ark.key_id).decode()}")
        print(f"  singnature valid = {self.ark_valid}{' (selfsigned)' if self.ark_valid is True else ''}")

        if not self.ark_valid and self.µ_arch != 'Naples':
            errors += 'ARK needs to be self signed if not Naples µArchitecture!\n'

        print(f"")
        print(f"#### ASK (Amd SEV Signing Key)")
        print(f"  key_id = {binascii.hexlify(self.ask.key_id).decode()}")
        print(f"  singnature valid = {self.ask_valid}")

        if not self.ask_valid:
            errors += 'ASK signature not valid!\n'

        print(f"")
        print(f"#### ASK ARK Certificate")
        print(f"  cert digest: {'known' if self.file_hash_known else 'unknown'}")
        print(f"  µArchitecture: {self.µ_arch}")

        if not self.file_hash_known:
            errors += 'ASK/ARK Certificate has an unknown digest!\n'

        if errors:
            raise RuntimeError(errors)

    def write_ask_as_x509_cert(self, filename, key_filename=None):

        if key_filename and os.path.exists(key_filename):
            # load ca key
            with open(key_filename, "rb") as file:
                ca_key = load_pem_private_key(
                    file.read(),
                    password=None,
                )
        else:
            # create ca key
            ca_key = rsa.generate_private_key(
                public_exponent=65537,
                key_size=self.ask.modulus_size,
            )

        # create and save key
        issuer = x509.Name([
            x509.NameAttribute(NameOID.ORGANIZATIONAL_UNIT_NAME, u"Engineering"),
            x509.NameAttribute(NameOID.COUNTRY_NAME, u"US"),
            x509.NameAttribute(NameOID.LOCALITY_NAME, u"Santa Clara"),
            x509.NameAttribute(NameOID.STATE_OR_PROVINCE_NAME, u"CA"),
            x509.NameAttribute(NameOID.ORGANIZATION_NAME, u"Advanced Micro Devices"),
            x509.NameAttribute(NameOID.COMMON_NAME, u"Dummy Amd Root Key"),
        ])

        subject = x509.Name([
            x509.NameAttribute(NameOID.ORGANIZATIONAL_UNIT_NAME, u"Engineering"),
            x509.NameAttribute(NameOID.COUNTRY_NAME, u"US"),
            x509.NameAttribute(NameOID.LOCALITY_NAME, u"Santa Clara"),
            x509.NameAttribute(NameOID.STATE_OR_PROVINCE_NAME, u"CA"),
            x509.NameAttribute(NameOID.ORGANIZATION_NAME, u"Advanced Micro Devices"),
            x509.NameAttribute(NameOID.COMMON_NAME, f"SEV-{self.µ_arch}"),
        ])

        cert = x509.CertificateBuilder().subject_name(
            subject
        ).issuer_name(
            issuer
        ).public_key(
            self.ask.public_key
        ).serial_number(
            x509.random_serial_number()
        ).not_valid_before(
            datetime.datetime.utcnow()
        ).not_valid_after(
            datetime.datetime.utcnow() + datetime.timedelta(days=10)
        ).add_extension(
            x509.BasicConstraints(ca=True, path_length=None), critical=True,
        ).sign(ca_key, utils.hashes.SHA384())

        with open(filename, "wb") as file:
            file.write(cert.public_bytes(
                encoding=Encoding.PEM,
            ))

        if key_filename and not os.path.exists(key_filename):
            # safe ca key
            with open(key_filename, "wb") as file:
                file.write(ca_key.private_bytes(
                    encoding=Encoding.PEM,
                    format=PrivateFormat.TraditionalOpenSSL,
                    encryption_algorithm=NoEncryption(),
                ))

class CekCert:
    def __init__(self, filename):
        self.file_bytes = read_file_bytes(filename)

        self.signed_bytes = self.file_bytes[:0x414]
        self.signature_bytes = self.file_bytes[0x41c:0x61c]
        # reverse signature
        self.signature_bytes = int.from_bytes(self.signature_bytes, 'little').to_bytes(0x200, 'big')

        x = self.file_bytes[0x14:0x5c]
        y = self.file_bytes[0x5c:0xa4]
        self.public_key = ec.EllipticCurvePublicNumbers(
            int.from_bytes(x, 'little'),
            int.from_bytes(y, 'little'),
            ec.SECP384R1()  # NIST-P384
        ).public_key()

    def write_pubkey_as_pem(self, filename):
        with open(filename, "wb") as file:
            file.write(self.public_key.public_bytes(
                Encoding.PEM,
                PublicFormat.SubjectPublicKeyInfo,
            ))

    def verify(self, data, signature):
        return verify_ecdsa_sha384_sig(
            self.public_key, data, signature
        )


class VcekCert:
    def __init__(self, filename):
        self.file_bytes = read_file_bytes(filename)
        self.x509_cert = x509.load_der_x509_certificate(self.file_bytes)

        self.public_key = self.x509_cert.public_key()
        self.signed_bytes = self.x509_cert.tbs_certificate_bytes
        self.signature_bytes = self.x509_cert.signature

    def write_pubkey_as_pem(self, filename):
        with open(filename, "wb") as file:
            file.write(self.public_key.public_bytes(
                Encoding.PEM,
                PublicFormat.SubjectPublicKeyInfo,
            ))

    def verify(self, data, signature):
        return verify_ecdsa_sha384_sig(
            self.public_key, data, signature
        )

TITLE = read_file_bytes('title.txt')
ID_NAPLES_HEX = 'ded5992ef2ed25e5325748671b6114e5cfac801d7420b3d289c833db2bf755ea735bbe1092e5cd33bc4e31af1a2880ad8d1fbf640c8f80dc02ba464e6c6c9395'
ID_MILAN_HEX = '91c44a33162d17b3a04ff6b631b6f6bc735b1b81cc103b69dced1c72b15a376c486a36da75e1d9239f1a18a62bd52b14a5e31af999472dbc068d22728181a0d7'

def verify_cek_title_sig(id_hex, ask):

    cek_cert_filename = f'certs/cek/{id_hex}.cert'
    cek_cert_url = f'https://kdsintf.amd.com/cek/id/{id_hex}'
    file_exists_or_download(cek_cert_filename, cek_cert_url)

    print()
    print(f"## Verifying {cek_cert_filename}")

    cek = CekCert(cek_cert_filename)

    print()
    if ask.check_cert(cek) is True:
        print("Verification OK!")
    else:
        print("Verification FAILED!")
        raise RuntimeError("cek certificate verification failed!")

    title_sig_filename = f'cek_signatures/{id_hex}.sig'

    print()
    print(f"## Verifying {title_sig_filename}")
    title_sig = read_file_bytes(title_sig_filename)

    print()
    if cek.verify(TITLE, title_sig) is True:
        print("Verification OK!")
    else:
        print("Verification FAILED!")
        raise RuntimeError("cek signature verification failed!")

def verify_vcek_title_sig(bl_ver, tee_ver, snp_ver, ucode_ver, ask):

    vcek_version_hex = hex(0x100 + bl_ver)[3:]
    vcek_version_hex += hex(0x100 + tee_ver)[3:]
    vcek_version_hex += hex(0x100 + snp_ver)[3:]
    vcek_version_hex += hex(0x100 + ucode_ver)[3:]

    vcek_cert_filename = f'certs/vcek/{vcek_version_hex}.cert'
    vcek_cert_url = f'https://kdsintf.amd.com/vcek/v1/Milan/{ID_MILAN_HEX}?blSPL={bl_ver}&teeSPL={tee_ver}&snpSPL={snp_ver}&ucodeSPL={ucode_ver}'
    file_exists_or_download(vcek_cert_filename, vcek_cert_url)

    print()
    print(f"## Verifying {vcek_cert_filename}")

    vcek = VcekCert(vcek_cert_filename)

    print()
    if ask.check_cert(vcek) is True:
        print("Verification OK!")
    else:
        print("Verification FAILED!")
        raise RuntimeError("cek certificate verification failed!")


    title_sig_filename = f'vcek_signatures/{vcek_version_hex}.sig'
    print()
    print(f"## Verifying {title_sig_filename}")

    try:
        title_sig = read_file_bytes(title_sig_filename)
    except FileNotFoundError:
        print("We couldn't generate all 2^32 signatures, this version combination doesn't exist!")
        raise

    print()
    if vcek.verify(TITLE, title_sig) is True:
        print("Verification OK!")
    else:
        print("Verification FAILED!")
        raise RuntimeError("vcek signature verification failed!")

if __name__ == "__main__":

    print()
    print("# ARK/ASK verification")


    ASK_ARK_MILAN_PATH = "certs/ask_ark_milan.cert"
    ASK_ARK_MILAN_URL = "https://developer.amd.com/wp-content/resources/ask_ark_milan.cert"

    ASK_MILAN_CA_PATH = "certs/ask_ark_milan.ca"

    ASK_ARK_ROME_PATH = "certs/ask_ark_rome.cert"
    ASK_ARK_ROME_URL = "https://developer.amd.com/wp-content/resources/ask_ark_rome.cert"


    file_exists_or_download(ASK_ARK_MILAN_PATH, ASK_ARK_MILAN_URL)
    file_exists_or_download(ASK_ARK_ROME_PATH, ASK_ARK_ROME_URL)


    ask_ark_milan = AmdAskArkCert(ASK_ARK_MILAN_PATH)
    ask_ark_milan.print_verification()


    #print()
    #print("#### Writing ASK as partial x509 cert to {ASK_MILAN_CA_CERT}")
    #ask_ark_milan.write_ask_as_x509_cert(ASK_MILAN_CA_PATH)


    ask_ark_rome = AmdAskArkCert(ASK_ARK_ROME_PATH)
    ask_ark_rome.print_verification()


    print()
    print("# CEK verification")

    verify_cek_title_sig(ID_NAPLES_HEX, ask_ark_rome.ask)
    verify_cek_title_sig(ID_MILAN_HEX, ask_ark_milan.ask)


    print()
    print("# VCEK verification")

    verify_vcek_title_sig(randint(0x00,0x07), randint(0x00,0x07), randint(0x00,0x07), randint(0x00,0x07), ask_ark_milan.ask)
    verify_vcek_title_sig(randint(0x40,0x47), randint(0x40,0x47), randint(0x40,0x47), randint(0x40,0x47), ask_ark_milan.ask)
    verify_vcek_title_sig(randint(0x78,0x7f), randint(0x78,0x7f), randint(0x78,0x7f), randint(0x78,0x7f), ask_ark_milan.ask)

    print()
    print("# VERIFICATION COMPLETE")


