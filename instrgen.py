#!/usr/bin/python2

f = open("instruction_set.h", "r")

lines = f.readlines()

instrs = []

for line in lines:
    if line[:6] == '    LI':
        line = line.strip()
        line = line.split(' = ')[0].split(',')[0]
        instrs.append(line)

print "void print_op(lky_instruction op)\n{\n    char *name = NULL;\n    switch(op)\n    {"

for istr in instrs:
    print "    case " + istr + ":"
    print '        name = "' + istr.split('LI_')[1] + '";\n        break;'

print '    default:\n' + r'        printf("   --> %d\n", op);' + '\n        return;\n    }\n'
print r'    printf("%s\n", name);'
print '}'
