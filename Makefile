COMPILER := gcc

ARTIFACT_PATH := build
BINARY_PATH   := bin
LOG_PATH      := .log
TEMP_PATH     := .temp

SOURCE_PATH    := src
CORE_PATH      := $(SOURCE_PATH)/core
FRONTEND_PATH  := $(SOURCE_PATH)/frontend
MIDDLEEND_PATH := $(SOURCE_PATH)/middleend
BACKEND_PATH   := $(SOURCE_PATH)/backend

INCLUDE_FLAGS := -I $(SOURCE_PATH)/ \
								 -I $(CORE_PATH)/
DEFINE_FLAGS  := -D _DEBUG \
							   -D LOG_STATUSES \
								 -D BACKEND_DEBUG_INFO \
								 -D CONDITIONAL_MOVES \
                 -D EASY_DIFFICULTY
								# -D HARD_DIFFICULTY
								# -D SIMPLIFIED_NODES
								# -D LOG_FORCE_TRACE
LIBS          := -lm -lc

MAIN_TARGET   := $(BINARY_PATH)/rpgc
FRONTEND      := $(BINARY_PATH)/rpgc-frontend
MIDDLEEND     := $(BINARY_PATH)/rpgc-middleend
BACKEND       := $(BINARY_PATH)/rpgc-backend
TODO_FILE     := TODO.txt

define to_object
$(patsubst $(SOURCE_PATH)/%.c, $(ARTIFACT_PATH)/%.o, $(1))
endef

COMPILER_MAIN_FILE := $(SOURCE_PATH)/main.c
MODULAR_MAIN_FILES := $(FRONTEND_PATH)/main.c  \
                      $(MIDDLEEND_PATH)/main.c \
                      $(BACKEND_PATH)/main.c

SOURCES_CORE       := $(shell find $(CORE_PATH)/      -type f -name '*.c')
SOURCES_FRONTEND   := $(shell find $(FRONTEND_PATH)/  -type f -name '*.c')
SOURCES_MIDDLEEND  := $(shell find $(MIDDLEEND_PATH)/ -type f -name '*.c')
SOURCES_BACKEND    := $(shell find $(BACKEND_PATH)/   -type f -name '*.c')
SOURCES            := $(SOURCES_CORE) $(SOURCES_FRONTEND) \
											$(SOURCES_MIDDLEEND) $(SOURCES_BACKEND) $(COMPILER_MAIN_FILE)

OBJECTS_CORE          := $(call to_object,$(SOURCES_CORE))
OBJECTS_FRONTEND      := $(call to_object,$(SOURCES_FRONTEND))
OBJECTS_MIDDLEEND     := $(call to_object,$(SOURCES_MIDDLEEND))
OBJECTS_BACKEND       := $(call to_object,$(SOURCES_BACKEND))
OBJECTS_MODULAR_MAINS := $(call to_object,$(MODULAR_MAIN_FILES))
OBJECTS               := $(OBJECTS_CORE) $(OBJECTS_FRONTEND) \
										     $(OBJECTS_MIDDLEEND) $(OBJECTS_BACKEND) $(call to_object,$(COMPILER_MAIN_FILE))

DEPENDENCIES := $(OBJECTS:.o=.d)

SANITIZER_FLAGS := -fsanitize=address,alignment,bool,bounds,enum,$\
		 		           float-cast-overflow,float-divide-by-zero,$\
				           integer-divide-by-zero,leak,nonnull-attribute,$\
				           null,object-size,return,returns-nonnull-attribute,$\
				           shift,signed-integer-overflow,undefined,$\
				           unreachable,vla-bound,vptr

C_FLAGS := -ggdb3 -O1 -Wall -Wextra                                       \
				   -Waggressive-loop-optimizations                                \
				   -Wmissing-declarations -Wcast-align -Wcast-qual                \
				   -Wchar-subscripts                                              \
				   -Wconversion  -Wempty-body                                     \
				   -Wfloat-equal -Wformat-nonliteral -Wformat-security            \
				   -Wformat-signedness -Wformat=2 -Winline -Wlogical-op           \
				   -Wopenmp-simd                                                  \
				   -Wpacked -Wpointer-arith -Winit-self -Wredundant-decls         \
				   -Wshadow -Wsign-conversion                                     \
				   -Wstrict-overflow=2 -Wsuggest-attribute=noreturn               \
				   -Wsuggest-final-methods -Wsuggest-final-types                  \
				   -Wswitch-default -Wsync-nand                                   \
				   -Wundef -Wunreachable-code -Wunused -Wuseless-cast             \
				   -Wvariadic-macros                                              \
				   -Wno-missing-field-initializers -Wno-narrowing                 \
				   -Wno-varargs -Wstack-protector                                 \
				   -fcheck-new -fstack-protector                                  \
				   -fstrict-overflow                                              \
				   -fno-omit-frame-pointer -Wlarger-than=64000                    \
				   -Wstack-usage=8192 -pie -fPIE -Werror=vla                      \
					 $(SANITIZER_FLAGS)
 
build: ensure_directories_exist $(FRONTEND) $(MIDDLEEND) $(BACKEND) $(MAIN_TARGET) update_todo

$(FRONTEND): $(OBJECTS_CORE) $(OBJECTS_FRONTEND)
	@echo -e "•Linking Frontend together"
	@$(COMPILER) $(C_FLAGS) $^ -o $@ $(LIBS)

$(MIDDLEEND): $(OBJECTS_CORE) $(OBJECTS_MIDDLEEND)
	@echo -e "•Linking Middleend together"
	@$(COMPILER) $(C_FLAGS) $^ -o $@ $(LIBS)

$(BACKEND): $(OBJECTS_CORE) $(OBJECTS_BACKEND)
	@echo -e "•Linking Backend together"
	@$(COMPILER) $(C_FLAGS) $^ -o $@ $(LIBS)

$(MAIN_TARGET): $(filter-out $(OBJECTS_MODULAR_MAINS),$(OBJECTS))
	@echo -e "•Linking RPGCompiler together"
	@$(COMPILER) $(C_FLAGS) $^ -o $@ $(LIBS)

-include $(DEPENDENCIES)

define declare_recipe
$(call to_object,$(1)): $(1)
endef

$(foreach $(SOURCE_PATH),$(SOURCES),$(eval $(strip $(call declare_recipe,$($(SOURCE_PATH))))))

%.o:
	@echo -e "•Compiling" $<
	@mkdir -p $(@D)
	@$(COMPILER) -c -MMD $(DEFINE_FLAGS) $(INCLUDE_FLAGS) $(LIBS) $(C_FLAGS) $< -o $@

%.d:

.PHONY: ensure_directories_exist clean build clean_logs update_todo

ensure_directories_exist:
	mkdir -p $(BINARY_PATH) $(ARTIFACT_PATH) $(LOG_PATH) $(TEMP_PATH)

clean:
	rm -f $(MAIN_TARGET) $(FRONTEND) $(MIDDLEEND) $(BACKEND)
	rm -f -r $(ARTIFACT_PATH)
	mkdir -p $(ARTIFACT_PATH)

clean_logs:
	rm -f -r $(LOG_PATH)
	mkdir -p $(LOG_PATH)

update_todo:
	@echo -e "•Updating $(TODO_FILE)"
	@rm -f $(TODO_FILE)
	@touch $(TODO_FILE)
	@grep -r -n "TODO" --exclude="Makefile" --exclude=".gitignore" --exclude="$(TODO_FILE)" --exclude-dir=.git | sed G >> $(TODO_FILE)
