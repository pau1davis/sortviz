CXX := g++
CXXFLAGS := -std=c++17 -O2 -Wall -Wextra -pedantic
TARGET := sortviz
SRC := generate_sort_jgraphs.cpp
DEMO_DIR := tests/demo
IMG_DIR := docs/images

.PHONY: all build demo clean

all: demo

build: $(TARGET)

demo: $(TARGET)
	mkdir -p $(DEMO_DIR) $(IMG_DIR)
	./$(TARGET) bubble 5 1 4 2 8
	bubble_dir=$$(ls -1dt tests/bin/bubble_* | head -n 1); \
	cp "$$bubble_dir/step_0001.pdf" $(DEMO_DIR)/bubble_swap.pdf; \
	cp "$$bubble_dir/step_0006.pdf" $(DEMO_DIR)/bubble_late_pass.pdf
	./$(TARGET) merge 38 27 43 3 9 82 10
	merge_dir=$$(ls -1dt tests/bin/merge_* | head -n 1); \
	cp "$$merge_dir/step_0003.pdf" $(DEMO_DIR)/merge_mid_merge.pdf; \
	cp "$$merge_dir/step_0012.pdf" $(DEMO_DIR)/merge_near_final.pdf
	./$(TARGET) quick 10 7 8 9 1 5
	quick_dir=$$(ls -1dt tests/bin/quick_* | head -n 1); \
	cp "$$quick_dir/step_0004.pdf" $(DEMO_DIR)/quick_partition.pdf; \
	cp "$$quick_dir/step_0010.pdf" $(DEMO_DIR)/quick_pivot_settle.pdf
	convert -density 180 "$(DEMO_DIR)/bubble_swap.pdf[0]" -quality 92 $(DEMO_DIR)/bubble_swap.jpg
	cp $(DEMO_DIR)/bubble_swap.jpg $(IMG_DIR)/bubble_swap.jpg
	convert -density 180 "$(DEMO_DIR)/bubble_late_pass.pdf[0]" -quality 92 $(DEMO_DIR)/bubble_late_pass.jpg
	cp $(DEMO_DIR)/bubble_late_pass.jpg $(IMG_DIR)/bubble_late_pass.jpg
	convert -density 180 "$(DEMO_DIR)/merge_mid_merge.pdf[0]" -quality 92 $(DEMO_DIR)/merge_mid_merge.jpg
	cp $(DEMO_DIR)/merge_mid_merge.jpg $(IMG_DIR)/merge_mid_merge.jpg
	convert -density 180 "$(DEMO_DIR)/merge_near_final.pdf[0]" -quality 92 $(DEMO_DIR)/merge_near_final.jpg
	cp $(DEMO_DIR)/merge_near_final.jpg $(IMG_DIR)/merge_near_final.jpg
	convert -density 180 "$(DEMO_DIR)/quick_partition.pdf[0]" -quality 92 $(DEMO_DIR)/quick_partition.jpg
	cp $(DEMO_DIR)/quick_partition.jpg $(IMG_DIR)/quick_partition.jpg
	convert -density 180 "$(DEMO_DIR)/quick_pivot_settle.pdf[0]" -quality 92 $(DEMO_DIR)/quick_pivot_settle.jpg
	cp $(DEMO_DIR)/quick_pivot_settle.jpg $(IMG_DIR)/quick_pivot_settle.jpg

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET)
	rm -rf $(DEMO_DIR)
