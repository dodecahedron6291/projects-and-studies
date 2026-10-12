CXX = g++
CXXFLAGS = -Wall -Wextra -g3 -O3 -fsanitize=address,undefined -std=c++23 -Werror -MMD -MP
OBJ = convert_to_wav.o file_read.o handle_events.o handle_headers.o note.o main.o

midi: $(OBJ)
	$(CXX) $(CXXFLAGS) $(OBJ) -o midi

convert_to_wav.o: convert_to_wav.cpp
	$(CXX) $(CXXFLAGS) -c convert_to_wav.cpp

file_read.o: file_read.cpp
	$(CXX) $(CXXFLAGS) -c file_read.cpp

handle_headers.o: handle_headers.cpp
	$(CXX) $(CXXFLAGS) -c handle_headers.cpp

handle_events.o: handle_events.cpp
	$(CXX) $(CXXFLAGS) -c handle_events.cpp

note.o: note.cpp
	$(CXX) $(CXXFLAGS) -c note.cpp

main.o: main.cpp
	$(CXX) $(CXXFLAGS) -c main.cpp

clean:
	rm -f *.o *.d midi ./test/*.wav

test: midi
	for f in test/*.mid; do echo "=== $$f ==="; ./midi "$$f"; done

-include *.d

.PHONY: clean test