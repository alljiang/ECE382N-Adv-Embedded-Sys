from subprocess import call
import os
import binascii
import random 
import pyaes, binascii, os
from subprocess import call
from typing import Tuple
import time

class UnitTestConfig:
    def __init__(self, 
                 test_name: str, 
                 key: str, 
                 iv: str):
        self.test_name = test_name
        self.key = key
        self.iv = iv

def accelerator_test(config: UnitTestConfig) -> Tuple[str, int]:
    cmd = f'./aes inputs/{config.test_name} {config.key} {config.iv} outputs/{config.test_name}'
    # print(cmd)
    time_diff = os.popen(f'./aes inputs/{config.test_name} {config.key} {config.iv} outputs/{config.test_name}').read()

    # call(['./aes', 'inputs/' + config.test_name, config.key, config.iv, 'outputs/' + config.test_name])

    with open('outputs/' + config.test_name, 'rb') as file:
        ciphertext = file.read()
        return (ciphertext.decode(), float(time_diff))

def aes_library_test(config: UnitTestConfig) -> Tuple[str, int]:
    start_time = time.time()
    with open('inputs/' + config.test_name, 'rb') as file:
        plaintext = file.read()
        counter = pyaes.Counter(initial_value=int(config.iv, 16))
        aes = pyaes.AESModeOfOperationCTR(
            binascii.unhexlify(config.key), counter=counter)
        ciphertext = aes.encrypt(plaintext)
        end_time = time.time()

        return (binascii.hexlify(ciphertext).decode(), (end_time - start_time)*1000000)

def generate_hex_string(length):
  hex_digits = "0123456789abcdef"
  result = ""
  for i in range(length):
    result += random.choice(hex_digits)
  return result

def generate_random_test_config(plaintext_length:int, key_size: int, test_name: str) -> UnitTestConfig:
    key = None
    if (key_size == 128):
        key = generate_hex_string(32)
    elif (key_size == 192):
        key = generate_hex_string(48)
    elif (key_size == 256):
        key = generate_hex_string(64)

    # generate random number between 0 and 2^128
    iv = generate_hex_string(32)

    with open('inputs/' + test_name, 'wb') as file:
        file.write(os.urandom(plaintext_length))
    
    return UnitTestConfig(test_name, key, iv)

def run_test_suite():
    string_lengths = [10, 100, 1000, 10000, 16000]
    key_sizes = [128, 192, 256]
        
    for strlen in string_lengths:
        for keysize in key_sizes:
            library_time_sum = 0
            accelerator_time_sum = 0

            for i in range(0, 10):
                my_test = generate_random_test_config(strlen, keysize, 'test.txt')

                library_ciphertext, library_time = aes_library_test(my_test)
                accelerator_ciphertext, accelerator_time = accelerator_test(my_test)

                library_time_sum += library_time
                accelerator_time_sum += accelerator_time

                if (library_ciphertext != accelerator_ciphertext):
                    print(f"Length: {strlen}, Keysize: {keysize}")
                    print("Test failed")
                    print(library_ciphertext)
                    print(accelerator_ciphertext)
                    exit()
            
            print(f"Length: {strlen}, Keysize: {keysize} | Library time: {library_time_sum/10} us, Accelerator time: {accelerator_time_sum/10} us")

run_test_suite()
