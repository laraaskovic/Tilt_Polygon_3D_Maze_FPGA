import struct
import sys

def convert(raw_filename, array_name, header_filename):
    with open(raw_filename, 'rb') as f:
        data = f.read()

    samples = []
    for i in range(0, len(data) - 1, 2):
        s16 = struct.unpack_from('<h', data, i)[0]  # read 16-bit signed sample
        s32 = s16 << 16                              # shift up to 32-bit for DE1-SoC
        samples.append(s32)

    with open(header_filename, 'w') as f:
        guard = array_name.upper() + '_H'
        f.write(f'#ifndef {guard}\n')
        f.write(f'#define {guard}\n\n')
        f.write(f'int {array_name}_len = {len(samples)};\n\n')
        f.write(f'int {array_name}[] = {{\n')

        for i, s in enumerate(samples):
            if i % 8 == 0:
                f.write('    ')
            f.write(f'{s}')
            if i < len(samples) - 1:
                f.write(', ')
            if (i + 1) % 8 == 0:
                f.write('\n')

        f.write('\n};\n\n')
        f.write(f'#endif\n')

    print(f'Done: {header_filename} — {len(samples)} samples ({len(samples)*4//1024} KB)')

convert('target.raw', 'snd_target', 'snd_target.h')
convert('wall.raw',   'snd_wall',   'snd_wall.h')
convert('reset.raw',  'snd_reset',  'snd_reset.h')
convert('agent.raw',  'snd_agent',  'snd_agent.h')