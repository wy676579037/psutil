/*
 * Copyright (c) 2009, Jay Loden, Giampaolo Rodola'. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
System-wide CPU related functions.
Original code was refactored and moved from psutil/arch/freebsd/specific.c
in 2020 (and was moved in there previously already) from cset.
a4c0a0eb0d2a872ab7a45e47fcf37ef1fde5b012
For reference, here's the git history with original(ish) implementations:
- CPU stats: fb0154ef164d0e5942ac85102ab660b8d2938fbb
- CPU freq: 459556dd1e2979cdee22177339ced0761caf4c83
- CPU cores: e0d6d7865df84dc9a1d123ae452fd311f79b1dde
*/


#include <Python.h>
#include <sys/sysctl.h>

#include "../../_psutil_common.h"
#include "../../_psutil_posix.h"


PyObject *
psutil_cpu_topology(PyObject *self, PyObject *args) {
    void *topology = NULL;
    size_t size = 0;
    PyObject *py_str;

    if (sysctlbyname("kern.sched.topology_spec", NULL, &size, NULL, 0))
        goto error;

    topology = malloc(size);
    if (!topology) {
        PyErr_NoMemory();
        return NULL;
    }

    if (sysctlbyname("kern.sched.topology_spec", topology, &size, NULL, 0))
        goto error;

    py_str = Py_BuildValue("s", topology);
    free(topology);
    return py_str;

error:
    if (topology != NULL)
        free(topology);
    Py_RETURN_NONE;
}


PyObject *
psutil_cpu_stats(PyObject *self, PyObject *args) {
    unsigned int v_soft;
    unsigned int v_intr;
    unsigned int v_syscall;
    unsigned int v_trap;
    unsigned int v_swtch;
    size_t size = sizeof(v_soft);

    if (sysctlbyname("vm.stats.sys.v_soft", &v_soft, &size, NULL, 0)) {
        return PyErr_SetFromOSErrnoWithSyscall(
            "sysctlbyname('vm.stats.sys.v_soft')");
    }
    if (sysctlbyname("vm.stats.sys.v_intr", &v_intr, &size, NULL, 0)) {
        return PyErr_SetFromOSErrnoWithSyscall(
            "sysctlbyname('vm.stats.sys.v_intr')");
    }
    if (sysctlbyname("vm.stats.sys.v_syscall", &v_syscall, &size, NULL, 0)) {
        return PyErr_SetFromOSErrnoWithSyscall(
            "sysctlbyname('vm.stats.sys.v_syscall')");
    }
    if (sysctlbyname("vm.stats.sys.v_trap", &v_trap, &size, NULL, 0)) {
        return PyErr_SetFromOSErrnoWithSyscall(
            "sysctlbyname('vm.stats.sys.v_trap')");
    }
    if (sysctlbyname("vm.stats.sys.v_swtch", &v_swtch, &size, NULL, 0)) {
        return PyErr_SetFromOSErrnoWithSyscall(
            "sysctlbyname('vm.stats.sys.v_swtch')");
    }

    return Py_BuildValue(
        "IIIII",
        v_swtch,  // ctx switches
        v_intr,  // interrupts
        v_soft,  // software interrupts
        v_syscall,  // syscalls
        v_trap  // traps
    );
}


/*
 * Return frequency information of a given CPU.
 * As of Dec 2018 only CPU 0 appears to be supported and all other
 * cores match the frequency of CPU 0.
 */
PyObject *
psutil_cpu_freq(PyObject *self, PyObject *args) {
    int current;
    int core;
    char sensor[26];
    char available_freq_levels[1000];
    size_t size = sizeof(current);

    if (! PyArg_ParseTuple(args, "i", &core))
        return NULL;
    // https://www.unix.com/man-page/FreeBSD/4/cpufreq/
    sprintf(sensor, "dev.cpu.%d.freq", core);
    if (sysctlbyname(sensor, &current, &size, NULL, 0))
        goto error;

    size = sizeof(available_freq_levels);
    // https://www.unix.com/man-page/FreeBSD/4/cpufreq/
    // In case of failure, an empty string is returned.
    sprintf(sensor, "dev.cpu.%d.freq_levels", core);
    sysctlbyname(sensor, &available_freq_levels, &size, NULL, 0);

    return Py_BuildValue("is", current, available_freq_levels);

error:
    if (errno == ENOENT)
        PyErr_SetString(PyExc_NotImplementedError, "unable to read frequency");
    else
        PyErr_SetFromErrno(PyExc_OSError);
    return NULL;
}


PyObject *
psutil_cpu_model(PyObject *self, PyObject *args) {
    void *buf = NULL;
    size_t size = 0;
    PyObject *py_str;

    if (sysctlbyname("hw.model", NULL, &size, NULL, 0))
        goto error;

    buf = malloc(size);
    if (!buf) {
        PyErr_NoMemory();
        return NULL;
    }

    if (sysctlbyname("hw.model", buf, &size, NULL, 0))
        goto error;

    py_str = Py_BuildValue("s", buf);
    free(buf);
    return py_str;

error:
    if (buf != NULL)
        free(buf);
    Py_RETURN_NONE;
}
