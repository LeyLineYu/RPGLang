COMPILER := gcc

SOURCE_PATH   := src
ARTIFACT_PATH := build
BINARY_PATH   := bin
LOG_PATH      := .log
TEMP_PATH     := .temp

INCLUDE_FLAGS := -I $(SOURCE_PATH)/ \
								 -I $(SOURCE_PATH)/core/
DEFINE_FLAGS  := -D _DEBUG \
							   -D LOG_STATUSES \
								 -D BACKEND_DEBUG_INFO \
								 -D CONDITIONAL_MOVES
								# -D HARD_DIFFICULTY
                # -D EASY_DIFFICULTY
								# -D SIMPLIFIED_NODES
								# -D LOG_FORCE_TRACE
LIBS          := -lm -lc

FRONTEND      := $(BINARY_PATH)/rpgc-frontend
MIDDLEEND     := $(BINARY_PATH)/rpgc-middleend
BACKEND       := $(BINARY_PATH)/rpgc-backend
TODO_FILE     := TODO.txt

define to_object
$(patsubst $(SOURCE_PATH)/%.c, $(ARTIFACT_PATH)/%.o, $(1))
endef

SOURCES_CORE      := $(shell find $(SOURCE_PATH)/core/ -type f -name '*.c')
SOURCES_FRONTEND  := $(shell find $(SOURCE_PATH)/frontend/ -type f -name '*.c' )
SOURCES_MIDDLEEND := $(shell find $(SOURCE_PATH)/middleend/ -type f -name '*.c')
SOURCES_BACKEND   := $(shell find $(SOURCE_PATH)/backend/ -type f -name '*.c')
SOURCES           := $(SOURCES_CORE) $(SOURCES_FRONTEND) $(SOURCES_MIDDLEEND) $(SOURCES_BACKEND)

OBJECTS_CORE      := $(call to_object,$(SOURCES_CORE))
OBJECTS_FRONTEND  := $(call to_object,$(SOURCES_FRONTEND))
OBJECTS_MIDDLEEND := $(call to_object,$(SOURCES_MIDDLEEND))
OBJECTS_BACKEND   := $(call to_object,$(SOURCES_BACKEND))
OBJECTS           := $(OBJECTS_CORE) $(OBJECTS_FRONTEND) $(OBJECTS_MIDDLEEND) $(OBJECTS_BACKEND)

DEPENDENCIES := $(OBJECTS:.o=.d)

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
				   -fsanitize=address,alignment,bool,bounds,enum,$\
				   float-cast-overflow,float-divide-by-zero,$\
				   integer-divide-by-zero,leak,nonnull-attribute,$\
				   null,object-size,return,returns-nonnull-attribute,$\
				   shift,signed-integer-overflow,undefined,$\
				   unreachable,vla-bound,vptr

build: ensure_directories_exist $(FRONTEND) $(MIDDLEEND) $(BACKEND) update_todo

$(FRONTEND): $(OBJECTS_CORE) $(OBJECTS_FRONTEND)
	@echo -e "•Linking Frontend together"
	@$(COMPILER) $(INCLUDE_FLAGS) $(C_FLAGS) $^ -o $@ $(LIBS)

$(MIDDLEEND): $(OBJECTS_CORE) $(OBJECTS_MIDDLEEND)
	@echo -e "•Linking Middleend together"
	@$(COMPILER) $(INCLUDE_FLAGS) $(C_FLAGS) $^ -o $@ $(LIBS)

$(BACKEND): $(OBJECTS_CORE) $(OBJECTS_BACKEND)
	@echo -e "•Linking Backend together"
	@$(COMPILER) $(INCLUDE_FLAGS) $(C_FLAGS) $^ -o $@ $(LIBS)

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
	rm -f $(PROGRAM_NAME)
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
