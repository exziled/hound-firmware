
WHO = $(USER)
#WHO = techplex

export SPARK_CORE_ID=
export SPARK_ACCESS_TOKEN=

ifeq ($(WHO), techplex)
export SPARK_CORE_ID=48ff6c065067555026311387
export SPARK_ACCESS_TOKEN=b122a221bf419da7491e4fca108f1835a2794451
endif

ifeq ($(WHO), bcarlson)
export SPARK_CORE_ID=53ff6d065067544847310187
export SPARK_ACCESS_TOKEN=e96d0e4094cde195f7bdbac1945d4a4c04a1d481
endif

ifeq ($(WHO), melinda)
export SPARK_CORE_ID=53ff6d065067544847310187
export SPARK_ACCESS_TOKEN=e96d0e4094cde195f7bdbac1945d4a4c04a1d481
endif

ifndef SPARK_CORE_ID
$(error SPARK_CORE_ID is undefined)
endif
ifndef SPARK_ACCESS_TOKEN
$(error SPARK_ACCESS_TOKEN is undefined)
endif


SUBDIRS = $(shell find libs -type d)

mkfile_path = $(abspath $(lastword $(MAKEFILE_LIST)))
current_dir = $(notdir $(patsubst %/,%,$(dir $(mkfile_path))))

export APP = $(current_dir)

export INCLUDE_DIRS += $(addprefix applications/$(current_dir)/, $(SUBDIRS))

DIR = ../../build

all:
	$(MAKE) -C $(DIR) all

flash:
	$(MAKE) -C $(DIR) all
	$(MAKE) -C $(DIR) program-cloud

dfu:
	$(MAKE) -C $(DIR) all
	-curl -m 3 https://api.spark.io/v1/devices/$(SPARK_CORE_ID)/dfu -d access_token=$(SPARK_ACCESS_TOKEN)
	$(MAKE) -C $(DIR) program-dfu

clean:
	$(MAKE) -C $(DIR) clean

test:

	@echo $(INCLUDE_DIRS)
	@echo $(current_dir)
	@echo $(SPARK_CORE_ID)
	@echo $(SPARK_ACCESS_TOKEN)

.phony: all flash dfu clean test
