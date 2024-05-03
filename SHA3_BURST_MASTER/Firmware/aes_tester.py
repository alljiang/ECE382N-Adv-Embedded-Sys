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
    print(cmd)
    start_time = time.time()
    os.system('./aes inputs/' + config.test_name + ' ' + config.key + ' ' + config.iv + ' outputs/' + config.test_name)
    end_time = time.time()

    # call(['./aes', 'inputs/' + config.test_name, config.key, config.iv, 'outputs/' + config.test_name])

    with open('outputs/' + config.test_name, 'rb') as file:
        ciphertext = file.read()
        return (ciphertext.decode(), end_time - start_time)

def aes_library_test(config: UnitTestConfig) -> Tuple[str, int]:
    start_time = time.time()
    with open('inputs/' + config.test_name, 'rb') as file:
        plaintext = file.read()
        counter = pyaes.Counter(initial_value=int(config.iv, 16))
        aes = pyaes.AESModeOfOperationCTR(
            binascii.unhexlify(config.key), counter=counter)
        ciphertext = aes.encrypt(plaintext)
        end_time = time.time()

        return (binascii.hexlify(ciphertext).decode(), end_time - start_time)

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
    # all_tests = []
    
    my_test = UnitTestConfig('test.txt', '00000000000000000000000000000000', '00000000000000000000000000000001')
    # my_test = generate_random_test_config(16, 128, 'test.txt')
    # print(my_test.iv)
    # print(my_test.key)
    library_ciphertext, library_time = aes_library_test(my_test)
    accelerator_ciphertext, accelerator_time = accelerator_test(my_test)

    print(library_ciphertext)
    print(accelerator_ciphertext)
    print(library_time, accelerator_time)

    if (library_ciphertext == accelerator_ciphertext):
        print("Test passed")
    else:
        print("Test failed")



# clear tests directory


run_test_suite()
