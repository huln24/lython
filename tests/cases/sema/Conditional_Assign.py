# >>> case: 0
# >>> code
c = True
if c:
    a = 2
b = a + 1
# <<<

# >>> case: 1
# >>> code
if False:
    a = 2
b = a + 1
# <<<

# >>> case: 2
# >>> code
if True:
    a = 2
else:
    a = 3
b = a + 1
# <<<

