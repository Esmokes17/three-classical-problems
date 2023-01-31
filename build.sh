#!/bin/sh

gcc bounded_buffer.c -lpthread -o bounded_buffer
gcc dinning_philosophers.c -lpthread -o dinning_philosopher
gcc writer_reader.c -lpthread -o writer_reader