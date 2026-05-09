# ─────────────────────────────────────────────
# LINX - Linux Process & Resource Manager
# Makefile
# ─────────────────────────────────────────────

CC      := gcc
CFLAGS  := -Wall -Wextra -std=c11 -g -Iinclude
LDFLAGS := -lpthread

TARGET  := linx
BUILDDIR := build

# Source files
SRC := src/main.c \
       src/process_mgr.c \
       src/scheduling/scheduler.c \
       src/memory/paging.c \
       src/memory/frame_alloc.c \
       src/sync/semaphore_demo.c \
       src/sync/buffer.c

OBJ := $(patsubst src/%.c, $(BUILDDIR)/%.o, $(SRC))

# ── Default target ────────────────────────────
all: $(BUILDDIR) $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
	@echo ""
	@echo "  [OK] Build successful - $(TARGET)"
	@echo ""

# ── Compile each .c to .o ─────────────────────
$(BUILDDIR)/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# ── Create build directory ────────────────────
$(BUILDDIR):
	@mkdir -p $(BUILDDIR)
	@mkdir -p $(BUILDDIR)/scheduling
	@mkdir -p $(BUILDDIR)/memory
	@mkdir -p $(BUILDDIR)/sync

# ── Clean ─────────────────────────────────────
clean:
	@rm -rf $(BUILDDIR)
	@rm -f $(TARGET)
	@rm -f linx.exe
	@echo "  [OK] Cleaned."

# ── Run ───────────────────────────────────────
run: all
	./$(TARGET)

# ── Run with Round Robin, 3 frames ───────────
run-rr: all
	./$(TARGET) --scheduler=rr --quantum=3 --frames=3

.PHONY: all clean run run-rr
