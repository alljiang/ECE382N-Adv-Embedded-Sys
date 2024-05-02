from subprocess import call
import os
import binascii
from Crypto.Cipher import AES
from Crypto.Util import Counter
from Crypto.Random import get_random_bytes
import random 
import pyaes, pbkdf2, binascii, os, secrets

class UnitTestConfig:
    def __init__(self, plaintext, key, iv):
        self.plaintext = plaintext
        self.key = key
        self.iv = iv

# Function to call the C program with given arguments
def call_c_program(plaintext, key, iv):
    # args = ['./aes', plaintext, ' ', key, ' ', iv, ' output.txt']  # Modify './aes_ctr_encrypt' to your C program's executable name
    # subprocess.run(args)
    call(['./aes', plaintext, key, iv, 'output.txt'])

# Function to read ciphertext from the output file
def read_ciphertext_from_file():
    with open('output.txt', 'rb') as file:
        ciphertext = file.read()
    return ciphertext

# Function to compute AES encryption in CTR mode using Python library
def aes_ctr_encrypt_python(plaintext, key, iv):
    # counter = Counter.new(128, initial_value=int(iv,16))
    
    # cipher = AES.new(key, AES.MODE_CTR, counter=counter)
    # ciphertext = cipher.encrypt(plaintext)
    # print("Encrypted: ", ciphertext)
    # return ciphertext
    iv = secrets.randbits(256)
    # plaintext = "Text for encryption"
    aes = pyaes.AESModeOfOperationCTR(key, pyaes.Counter(iv))
    ciphertext = aes.encrypt(plaintext)
    print("Encrypted data: ", ciphertext)
    print('Encrypted:', binascii.hexlify(ciphertext).decode())
    return binascii.hexlify(ciphertext).decode()

# Function to compare two ciphertexts
def compare_ciphertexts(ciphertext1, ciphertext2):
    if ciphertext1 == ciphertext2:
        return True
    else:
        return False

# Main function to test AES encryption with random inputs
def test_aes_ctr_encryption():
    passing_tests = 0
    failing_tests = 0

    # Number of tests per byte length
    num_tests_per_length = 5

    for length in range(1, 17):  # Test plaintexts of length 1 to 16 bytes
        print("Testing plaintext length: ", length)
        for i in range(num_tests_per_length):
            # Generate random plaintext, key, and IV
            plaintext = "1234567812345678" #get_random_bytes(length)
            key = "00112233445566778899AABBCCDDEEFF00112233445566778899AABBCCDDEEFF" #get_random_bytes(32)  # 256-bit key
            # iv = get_random_bytes(16)   # 128-bit IV
            iv =  "00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000" #"00000000" * 16
            # print("IV: ", iv)
            # Call C program to encrypt
            # call_c_program(binascii.hexlify(plaintext).decode(), binascii.hexlify(key).decode(), binascii.hexlify(iv).decode())
            # call_c_program(binascii.hexlify(plaintext).decode(), binascii.hexlify(key).decode(), binascii.hexlify(iv).decode())
            call_c_program(plaintext, key, iv)
            
            # Read ciphertext from output file
            ciphertext_c = read_ciphertext_from_file()
            
            # Compute ciphertext using Python library
            ciphertext_python = aes_ctr_encrypt_python(plaintext, key, iv)
            
            # Compare ciphertexts
            if compare_ciphertexts(ciphertext_c, ciphertext_python):
                print("Test passed")
                passing_tests += 1
            else:
                print("Test failed")
                failing_tests += 1

    print("\nTest results:")
    print("Passing tests: ", passing_tests)
    print("Failing tests: ", failing_tests)

# Call the main function to start testing
test_aes_ctr_encryption()
