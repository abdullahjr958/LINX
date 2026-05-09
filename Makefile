# ─────────────────────────────────────────────
# LINX - Linux Process & Resource Manager
# Makefile
# ─────────────────────────────────────────────

CC      := gcc
CFLAGS  := -Wall -Wextra -std=c11 -g -Iinclude
LDFLAGS := -lpthread

TARGET  := linx.exe
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
	@echo.
	@echo   [OK] Build successful - $(TARGET)
	@echo.

# ── Compile each .c to .o ─────────────────────
$(BUILDDIR)/%.o: src/%.c
	@if not exist "$(dir $@)" mkdir "$(dir $@)"
	$(CC) $(CFLAGS) -c $< -o $@

# ── Create build directory ────────────────────
$(BUILDDIR):
	@if not exist "$(BUILDDIR)" mkdir "$(BUILDDIR)"
	@if not exist "$(BUILDDIR)\scheduling" mkdir "$(BUILDDIR)\scheduling"
	@if not exist "$(BUILDDIR)\memory" mkdir "$(BUILDDIR)\memory"
	@if not exist "$(BUILDDIR)\sync" mkdir "$(BUILDDIR)\sync"

# ── Clean ─────────────────────────────────────
clean:
	@if exist "$(BUILDDIR)" rmdir /S /Q "$(BUILDDIR)"
	@if exist "$(TARGET)" del /Q "$(TARGET)"
	@if exist "linx" del /Q "linx"
	@echo   [OK] Cleaned.

# ── Run ───────────────────────────────────────
run: all
	$(TARGET)

# ── Run with Round Robin, 3 frames ───────────
run-rr: all
	$(TARGET) --scheduler=rr --quantum=3 --frames=3

.PHONY: all clean run run-rr
