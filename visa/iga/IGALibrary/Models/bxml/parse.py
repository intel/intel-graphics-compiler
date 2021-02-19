file1 = open('Model12P5.hpp', 'r')
Lines = file1.readlines()

new_file = open('Model12P5_new.hpp', 'w')

count = 0
# read old lines
for line in Lines:
    if "ENCODED(" not in line:
        new_file.writelines(line)
    else:
        # remove everything after "ENCODED("
        en_idx = line.find('ENCODED(')
        nstr = line[:en_idx-1]
        # remove '/' and '*' and ' '
        nstr = nstr.replace('/','')
        nstr = nstr.replace('*','')
        #nstr = nstr.replace(' ','')
        # count number of spaces
        n_spaces = 0
        for i in range(0, len(nstr)):
            if nstr[i] != ' ':
                # find first non-space, break
                break
            else:
                n_spaces = n_spaces + 1
        # replace last space before string to "
        nstr = nstr[:n_spaces-1] + "\"" + nstr[n_spaces:]
        # replace last space in string to "
        nstr = nstr[:len(nstr)-1] + "\""

        # add ',' at the end
        nstr = nstr + ",\n"
        new_file.writelines(nstr)



