COMPILER := gcc

ARTIFACT_PATH := build
BINARY_PATH   := bin
LOG_PATH      := .log

SOURCE_PATH     := src
CORE_PATH       := $(SOURCE_PATH)/core
CORE_DEBUG_PATH := $(CORE_PATH)/debug
FRONTEND_PATH   := $(SOURCE_PATH)/frontend
MIDDLEEND_PATH  := $(SOURCE_PATH)/middleend
BACKEND_PATH    := $(SOURCE_PATH)/backend

INCLUDE_FLAGS := -I $(SOURCE_PATH)/ \
								 -I $(CORE_PATH)/
LIBS          := -lm -lc
# Common defines
DEFINE_FLAGS  := -D CONDITIONAL_MOVES

# Debug exclusive defines
DEBUG_DEFINE_FLAGS := -D _DEBUG \
                      -D LOG_STATUSES \
								      -D BACKEND_DEBUG_INFO
								    # -D SIMPLIFIED_NODES
								    # -D LOG_FORCE_TRACE
										# -D PARSER_DEBUG_INFO

# Release exclusive defines
RELEASE_DEFINE_FLAGS := -D NDEBUG

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

SOURCES_CORE       := $(shell find $(CORE_PATH)/       -type f -name '*.c')
SOURCES_DEBUG_CORE := $(filter $(CORE_DEBUG_PATH)/%,$(SOURCES_CORE))
SOURCES_FRONTEND   := $(shell find $(FRONTEND_PATH)/   -type f -name '*.c')
SOURCES_MIDDLEEND  := $(shell find $(MIDDLEEND_PATH)/  -type f -name '*.c')
SOURCES_BACKEND    := $(shell find $(BACKEND_PATH)/    -type f -name '*.c')
SOURCES            := $(SOURCES_CORE) $(SOURCES_FRONTEND) \
											$(SOURCES_MIDDLEEND) $(SOURCES_BACKEND) $(COMPILER_MAIN_FILE)

OBJECTS_CORE          := $(call to_object,$(SOURCES_CORE))
OBJECTS_DEBUG_CORE    := $(call to_object,$(SOURCES_DEBUG_CORE))
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
# Debug exclusive flags
DEBUG_C_FLAGS   := -ggdb3 -O0 \
                   -Wstack-protector -fstack-protector \
									 -fno-omit-frame-pointer \
									 $(SANITIZER_FLAGS)
# Release exclusive flags
RELEASE_C_FLAGS := -O3

C_FLAGS := -Wall -Wextra                                                  \
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
				   -Wsync-nand                                                    \
				   -Wundef -Wunreachable-code -Wunused -Wuseless-cast             \
				   -Wvariadic-macros                                              \
				   -Wno-missing-field-initializers -Wno-narrowing                 \
				   -Wno-varargs -fstrict-overflow                                 \
				   -Wstack-usage=8192 -pie -fPIE -Werror=vla

.PHONY: debug debug_prehook release

debug: debug_prehook build

debug_prehook:
	$(eval DEFINE_FLAGS += $(DEBUG_DEFINE_FLAGS))
	$(eval C_FLAGS      += $(DEBUG_C_FLAGS))
	@echo "Debug mode"

release:
	$(eval DEFINE_FLAGS += $(RELEASE_DEFINE_FLAGS))
	$(eval C_FLAGS      += $(RELEASE_C_FLAGS))
	$(eval OBJECTS_CORE := $(filter-out $(OBJECTS_DEBUG_CORE),$(OBJECTS_CORE)))
	$(eval OBJECTS      := $(filter-out $(OBJECTS_DEBUG_CORE),$(OBJECTS)))
	@echo "Release mode"
	@# relaunch make with new variables set because 
	@# you cannot redefine recipe's ingridents at this stage
	@$(MAKE) build DEFINE_FLAGS="$(DEFINE_FLAGS)" \
	  	           C_FLAGS="$(C_FLAGS)" \
		 						 OBJECTS_CORE="$(OBJECTS_CORE)" OBJECTS="$(OBJECTS)"

build: ensure_directories_exist $(FRONTEND) $(MIDDLEEND) $(BACKEND) $(MAIN_TARGET) update_todo

$(FRONTEND): $(OBJECTS_CORE) $(OBJECTS_FRONTEND)
	@echo -e "\t• Linking Frontend together"
	@$(COMPILER) $(C_FLAGS) $^ -o $@ $(LIBS)

$(MIDDLEEND): $(OBJECTS_CORE) $(OBJECTS_MIDDLEEND)
	@echo -e "\t• Linking Middleend together"
	@$(COMPILER) $(C_FLAGS) $^ -o $@ $(LIBS)

$(BACKEND): $(OBJECTS_CORE) $(OBJECTS_BACKEND)
	@echo -e "\t• Linking Backend together"
	@$(COMPILER) $(C_FLAGS) $^ -o $@ $(LIBS)

$(MAIN_TARGET): $(filter-out $(OBJECTS_MODULAR_MAINS),$(OBJECTS))
	@echo -e "\t• Linking RPGCompiler together"
	@$(COMPILER) $(C_FLAGS) $^ -o $@ $(LIBS)

-include $(DEPENDENCIES)

define declare_recipe
$(call to_object,$(1)): $(1)
endef

$(foreach $(SOURCE_PATH),$(SOURCES),$(eval $(strip $(call declare_recipe,$($(SOURCE_PATH))))))

%.o:
	@echo -e "• Compiling" $<
	@mkdir -p $(@D)
	@$(COMPILER) -c -MMD $(DEFINE_FLAGS) $(INCLUDE_FLAGS) $(LIBS) $(C_FLAGS) $< -o $@

%.d:

.PHONY: ensure_directories_exist clean build clean_logs update_todo

ensure_directories_exist:
	mkdir -p $(BINARY_PATH) $(ARTIFACT_PATH) $(LOG_PATH) 

clean:
	rm -f $(MAIN_TARGET) $(FRONTEND) $(MIDDLEEND) $(BACKEND)
	rm -f -r $(ARTIFACT_PATH)
	mkdir -p $(ARTIFACT_PATH)

clean_logs:
	rm -f -r $(LOG_PATH)
	mkdir -p $(LOG_PATH)

update_todo:
	@echo -e "• Updating $(TODO_FILE)"
	@rm -f $(TODO_FILE)
	@touch $(TODO_FILE)
	@grep -r -n "TODO" --exclude="Makefile" --exclude=".gitignore" --exclude="$(TODO_FILE)" --exclude-dir=.git | sed G >> $(TODO_FILE)
