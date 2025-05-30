target_arch_map = {
    "dada" : "sun",
}

targets = [
    # keep sorted
    "monaco",
    "parrot",
    "pineapple",
    "sun",
    "dada",
]

la_variants = [
    # keep sorted
    "consolidate",
    "perf",
]

le_targets = [
    # keep sorted
    "sdxkova",
    "sun-allyes",
]

le_variants = [
    # keep sorted
    "debug-defconfig",
    "perf-defconfig",
]

vm_types = [
    "tuivm",
    "oemvm",
]

vm_target_bases = [
    "pineapple",
    "sun",
]

vm_targets = ["{}-{}".format(t, vt) for t in vm_target_bases for vt in vm_types]

vm_variants = [
    # keep sorted
    "debug-defconfig",
    "defconfig",
]

def get_all_la_variants():
    return [(t, v) for t in targets for v in la_variants]

def get_all_le_variants():
    return [(t, v) for t in le_targets for v in le_variants]

def get_all_vm_variants():
    return [(t, v) for t in vm_targets for v in vm_variants]

def get_all_non_la_variants():
    return get_all_le_variants() + get_all_vm_variants()

def get_all_variants():
    return get_all_la_variants() + get_all_le_variants() + get_all_vm_variants()

def get_arch_of_target(target):
    arch = target_arch_map.get(target)
    if not arch:
        arch = target
    return arch
