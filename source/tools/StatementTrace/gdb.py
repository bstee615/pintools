fault_location = 'test.c:10'

locals_decl='benjis:Locals'
globals_decl='benjis:Globals'
gdb_cmds = [
    f'b {fault_location}',
    'r',
    f'printf "{locals_decl}\n"',
    'info locals',
    f'printf "{globals_decl}\n"',
    'info variables',
    'd 1',
    'c',
    'q',
]
ex_gdb_args = [ex for cmd in gdb_cmds for ex in ('-ex', cmd)]

filename = './a.out'

import subprocess
import re
result = subprocess.run(['gdb', '-batch'] + ex_gdb_args + [filename], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
stdout = result.stdout.decode('utf-8')
i = iter(stdout.splitlines())
variable_values = []
for line in i:
    if line == locals_decl:
        line = next(i)
        m = re.match(r'(.*) = (.*)', line)
        while m:
            decl = (m.group(1), m.group(2))
            variable_values.append(decl)
            print(decl)
            line = next(i)
            m = re.match(r'(.*) = (.*)', line)
        while line != globals_decl:
            line = next(i)
        while line != 'Non-debugging symbols:':
            m = re.match(r'[0-9]+:\s+(.*)', line)
            if m:
                print('global', m.group(1))
            line = next(i)
        break
