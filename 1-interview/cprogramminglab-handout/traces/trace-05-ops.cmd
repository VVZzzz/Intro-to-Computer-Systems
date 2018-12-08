# Test of insert_head, insert_tail, remove_head reverse, and size
option fail 0
option malloc 0
new
ih 2
ih 1
ih 3
reverse
size
it 5
it 1
it 3
size
rh 2
reverse
size
rh 3
rh 1
rh 5
rh 3
rh 1
size
free
