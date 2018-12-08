# Test of insert_head, insert_tail, reverse, and remove_head
option fail 0
option malloc 0
new
ih 2
ih 1
ih 3
reverse
it 5
it 1
it 3
reverse
it 7
reverse
rh 7
ih 8
reverse
rh 3
rh 1
rh 5
rh 3
rh 1
rh 2
rh 8
