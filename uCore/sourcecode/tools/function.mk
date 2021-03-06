# 阅读以及理解注释都由个人完成，如有错误可以联系lumaoxin@aliyun.com
OBJPREFIX	:= __objs_

.SECONDEXPANSION:
# -------------------函数开始 --------------------

# 列举指定目录下的所有指定类型的文件 (#directories, #types)
# [分析]filter函数 先指明过滤规则，在指明过滤内容
# addperfix函数,如果指明了type参数的话,将.作为前缀加入到#type参数之前，构成%.type的类型，作为filter的过滤规则
# 如果没有指明type参数的话，就直接返回通配符
# $(SLASH)是在Makefile主文件定义的斜线,这里为目录名添加后缀，#directories/*
# 经过函数处理的返回的是连带通配符(bash)的字串，不是文件名称，所以需要wildcard函数来进行转化
listf = $(filter $(if $(2),$(addprefix %.,$(2)),%),\
		  $(wildcard $(addsuffix $(SLASH)*,$(1))))

# 传入文件列表得到所有的.o文件: (#files[, packet])
#$(OBJDIR)是定义在Makefile主文件的变量，OBJDIR	:= obj
# basename是取前缀函数,将其传入的后缀都干掉：$(basename src/foo.c src-1.0/bar.c hacks)返回值是“src/foo src-1.0/bar hacks”。
# 传入的文件参数都会去掉原有的后缀名之后加上.o作为后缀，之后再加上前缀
# 前缀的规则是如果#packet为空的话就是obj/,否则是obj/packet名/xxx.o
toobj = $(addprefix $(OBJDIR)$(SLASH)$(if $(2),$(2)$(SLASH)),\
		$(addsuffix .o,$(basename $(1))))

# 将.o文件转化位.d 文件(#files[, packet])
# patsubst 模式字符串替换函数,将%.o的文件转换为%.d的文件
# %.d的文件列表来自上面写的toobj函数
todep = $(patsubst %.o,%.d,$(call toobj,$(1),$(2)))

#为传入的变量添加bin/前缀
#$(BINDIR)这个变量来源于Makefile主文件 BINDIR	:= bin
totarget = $(addprefix $(BINDIR)$(SLASH),$(1))

# 将 $(name) 转化为 $(OBJPREFIX)$(name)或者(#names)
# 其中 OBJPREFIX:=__objs_
# 该函数的目的是将文件名添加__objs_前缀的形式,如果传入为空就只返回__objs_
packetname = $(if $(1),$(addprefix $(OBJPREFIX),$(1)),$(OBJPREFIX))

# cc 编译模版, 为dep, obj文件生成规则: (file, cc[, flags, dir])
# 在define --endef定义块中
# cc_template 在被调用之后，接受参数，组成一个makefile格式的文本，之后再去经过make
# 的解释执行
define cc_template
$$(call todep,$(1),$(4)): $(1) | $$$$(dir $$$$@)
	@$(2) -I$$(dir $(1)) $(3) -MM $$< -MT "$$(patsubst %.d,%.o,$$@) $$@"> $$@
$$(call toobj,$(1),$(4)): $(1) | $$$$(dir $$$$@)
	@echo + cc $$<
	$(V)$(2) -I$$(dir $(1)) $(3) -c $$< -o $$@
ALLOBJS += $$(call toobj,$(1),$(4))
endef

# compile file: (#files, cc[, flags, dir])
define do_cc_compile
$$(foreach f,$(1),$$(eval $$(call cc_template,$$(f),$(2),$(3),$(4))))
endef

# 将文件加入到包中: (#files, cc[, flags, packet, dir])
define do_add_files_to_packet
__temp_packet__ := $(call packetname,$(4))
ifeq ($$(origin $$(__temp_packet__)),undefined)
$$(__temp_packet__) :=
endif
__temp_objs__ := $(call toobj,$(1),$(5))
$$(foreach f,$(1),$$(eval $$(call cc_template,$$(f),$(2),$(3),$(5))))
$$(__temp_packet__) += $$(__temp_objs__)
endef

# add objs to packet: (#objs, packet)
define do_add_objs_to_packet
__temp_packet__ := $(call packetname,$(2))
ifeq ($$(origin $$(__temp_packet__)),undefined)
$$(__temp_packet__) :=
endif
$$(__temp_packet__) += $(1)
endef

# add packets and objs to target (target, #packes, #objs[, cc, flags])
define do_create_target
__temp_target__ = $(call totarget,$(1))
__temp_objs__ = $$(foreach p,$(call packetname,$(2)),$$($$(p))) $(3)
TARGETS += $$(__temp_target__)
ifneq ($(4),)
$$(__temp_target__): $$(__temp_objs__) | $$$$(dir $$$$@)
	$(V)$(4) $(5) $$^ -o $$@
else
$$(__temp_target__): $$(__temp_objs__) | $$$$(dir $$$$@)
endif
endef

# finish all
define do_finish_all
ALLDEPS = $$(ALLOBJS:.o=.d)
$$(sort $$(dir $$(ALLOBJS)) $(BINDIR)$(SLASH) $(OBJDIR)$(SLASH)):
	@$(MKDIR) $$@
endef

# --------------------  函数结束  --------------------
# compile file: (#files, cc[, flags, dir])
cc_compile = $(eval $(call do_cc_compile,$(1),$(2),$(3),$(4)))

# add files to packet: (#files, cc[, flags, packet, dir])
add_files = $(eval $(call do_add_files_to_packet,$(1),$(2),$(3),$(4),$(5)))

# add objs to packet: (#objs, packet)
add_objs = $(eval $(call do_add_objs_to_packet,$(1),$(2)))

# add packets and objs to target (target, #packes, #objs, cc, [, flags])
create_target = $(eval $(call do_create_target,$(1),$(2),$(3),$(4),$(5)))

read_packet = $(foreach p,$(call packetname,$(1)),$($(p)))

add_dependency = $(eval $(1): $(2))

finish_all = $(eval $(call do_finish_all))

