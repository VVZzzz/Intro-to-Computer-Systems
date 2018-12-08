# Demonstration of queue testing framework
# Use help command to see list of commands and options
# Initial queue is NULL.
show
# Create empty queue
new
# Fill it with some values.  First at the head
ih 2
ih 1
ih 3
# Now at the tail
it 5
it 1
# Reverse it
reverse
# See how long it is
size
# Delete queue.  Goes back to a NULL queue.
free
# Exit program
quit
