# Proof of key extraction

We extracted multiple cryptographic endorsement keys from the AMD-SPs and provide the signed title of our paper as proof.
In the following we explain how these signatures can be validated.

## TL;DR

Use the python script `verify.py`:
```
$ cd attack-proof
$ python verify.py
...

Verification OK!

# VERIFICATION COMPLETE
```

## Keys we extracted

 * *Chip Endoresement Keys* for Amd Epyc CPUs:
    * 7272 Rome (Zen2)
    * 72F3 Milan (Zen3)
 * *Versioned Chip Endoresement Keys* for the Amd Epyc 72F3 CPU (Milan/Zen3) with any *Security Version Numbers* (SVNs).

**Todo:** Why are these keys important for SEV paragraph and link.

## Signatures provided

We used the keys to sign the title of our paper (the file [title.txt](attack-proof/title.txt)).
The two CEK signatures can be found in [cek_signatures](attack-proof/cek_signatures) and the VCEK signatures we created in [vcek_signatures](attack-proof/vcek_signatures).

Since there are 2 to the power of 28 many VCEKs (we can only download certificates for SVNs between 0 and 0x7f), we didn't want to provide signatures for all of them.
We provided the signatures for:
 * all combinatins of SVNs between 0x00 and 0x08
 * all combinatins of SVNs between 0x40 and 0x48
 * all combinatins of SVNs between 0x78 and 0x7f

## Certificate chain

The CEKs and VCEKs can be trusted through the following trust chain:

 * *AMD Root Signing Key (ARK)*: a 4096 bit RSA key
 * *AMD SEV Signing Key (ASK)*: a 4096 bit RSA key signed with the ARK
 * *(Versioned) Chip Endorsement Key ((V)CEK)*: a 384 bit EC key signed with the ASK

### Obtaining Keys and Certificates

The ARK and ASK are seperate for each µArchitecture generation and their public parts and certificates can be downloaded from AMD [here: https://developer.amd.com/sev/](https://developer.amd.com/sev/).

The CEKs and VCEKs are identified by the so-called *ID* which consists 512 hex-encoded bits.
The IDs for our CPUs are:

 * 7272 Rome (Zen2): `ded5992ef2ed25e5325748671b6114e5cfac801d7420b3d289c833db2bf755ea735bbe1092e5cd33bc4e31af1a2880ad8d1fbf640c8f80dc02ba464e6c6c9395`
 * 72F3 Milan (Zen3): `91c44a33162d17b3a04ff6b631b6f6bc735b1b81cc103b69dced1c72b15a376c486a36da75e1d9239f1a18a62bd52b14a5e31af999472dbc068d22728181a0d7`

These can be used [here: https://kdsintf.amd.com/cek/](https://kdsintf.amd.com/cek/) to obtain the CEK certificate.
To obtain the VCEK [this: https://kdsintf.amd.com/vcek/](https://kdsintf.amd.com/vcek/) site is used with "Milan" as the product string.
The VCEK depends on four *Security Version Numbers* (SVNs) and to obtain the VCEK for a specific version one can use the url:
```
https://kdsintf.amd.com/vcek/v1/Milan/{id as hex string}?blSPL={Bootloader SVN}&teeSPL={Trusted-Exe.-Env. SVN}&snpSPL={SNP Firmware SVN}&ucodeSPL={µCode SVN}
```

We provide the ASK/ARK, CEK and VCEK certificates that are needed for verification in the [attack-proof/certs](attack-proof/certs) folder.
This is so that you can still check our signatures offline and in the case that AMD chooses to not provide the certificates for the compromised keys.

Our verification script downloads the certificates from AMD if they are not present under `certs/ask_ark_(naples|milan).cert`.
So you are welcome to delete them and see how the script downloads them, or even download them manually.

### Certificate Formats

The ARK, ASK and CEK certificates are provided in AMDs own formats specified in their [Secure Encrypted Virtualization API](https://www.amd.com/system/files/TechDocs/55766_SEV-KM_API_Specification.pdf).
The VCEK certificates are DER encoded x509 certificates.

### Verification

Our verification script works through the following steps:
 * download ASK/ARK ceritificates if not present
 * verify the ASK/ARK certificates agains known hashes
 * verify the ASK/ARK signatures
 * for both CEK signatures:
    * download the CEK cettificate if not present
    * verify the CEK certificate using the ASK certificate
    * verify the signed title using the CEK certificate
 * for one VCEK signature in each group (SVNs 0x00 to 0x08, 0x40 to 0x48, 0x78 to 0x7f)
    * download the VCEK cettificate if not present
    * verify the VCEK certificate using the ASK certificate
    * verify the signed title using the VCEK certificate

##### Manual verification of our signatures

Due to the custom formats used by AMD you will have to take a look into our python code and into AMDs specification to verify that our python code does what is says it does.
But since the VCEKs are available in a convenient x509 format, we can use the openssl tool to verify the signatures we generated.

First we have to extract the VCEK public part from the x509 certificate:
```
$ openssl x509 -inform DER -in certs/vcek/xxxxxxxx.cert -noout -pubkey -out /tmp/vcek_xxxxxxxx.pem
```
The public key can then be used to verify the signatures:
```
$ openssl dgst -verify /tmp/vcek_xxxxxxxx.pem -sha384 -signature vcek_signatures/xxxxxxxx.sig title.txt
Verified OK
```

We have also provided a shell script that checks all of the provided signatures under [attack-proof/check_vceks_with_openssl.sh](attack-proof/check_vceks_with_openssl.sh).


