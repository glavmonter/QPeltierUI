import sys
import Wake

in_data = sys.argv[1].replace(':', '')

bytes_data = bytes.fromhex(in_data)
cc = Wake.wake_check_crc(Wake.wake_decode(bytes_data))
print(f'Result: {cc}')
