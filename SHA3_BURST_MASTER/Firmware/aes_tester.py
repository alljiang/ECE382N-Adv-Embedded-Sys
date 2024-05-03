from subprocess import call
import os
import binascii
import random 
import pyaes, binascii, os
from subprocess import call

class UnitTestConfig:
    def __init__(self, 
                 test_name: str, 
                 key: str, 
                 iv: str):
        self.test_name = test_name
        self.key = key
        self.iv = iv

def accelerator_test(config: UnitTestConfig) -> str:
    cmd = f'./aes inputs/{config.test_name} {config.key} {config.iv} outputs/{config.test_name}'
    print(cmd)
    os.system('./aes inputs/' + config.test_name + ' ' + config.key + ' ' + config.iv + ' outputs/' + config.test_name)
    # call(['./aes', 'inputs/' + config.test_name, config.key, config.iv, 'outputs/' + config.test_name])

    with open('outputs/' + config.test_name, 'rb') as file:
        ciphertext = file.read()
        return ciphertext.decode()

def aes_library_test(config: UnitTestConfig):
    with open('inputs/' + config.test_name, 'rb') as file:
        plaintext = file.read()
    
        iv = int(config.iv, 16)
        counter = pyaes.Counter(initial_value=int(config.iv, 16))
        aes = pyaes.AESModeOfOperationCTR(
            binascii.unhexlify(config.key), counter=counter)
        ciphertext = aes.encrypt(plaintext)

        return binascii.hexlify(ciphertext).decode()

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
    
    my_test = UnitTestConfig('test.txt', '12345678123456781234567812345678', '0')
    correct_ciphertext = aes_library_test(my_test)
    accelerator_ciphertext = accelerator_test(my_test)

    print(correct_ciphertext)
    print(accelerator_ciphertext)

    if (correct_ciphertext == accelerator_ciphertext):
        print("Test passed")
    else:
        print("Test failed")



# clear tests directory


run_test_suite()
