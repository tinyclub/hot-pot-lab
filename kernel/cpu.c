#include <linux/types.h>
#include <base/cpumask.h>

const DECLARE_BITMAP(cpu_all_bits, NR_CPUS) = CPU_BITS_ALL;

static DECLARE_BITMAP(cpu_possible_bits, CONFIG_NR_CPUS) ;
const struct cpumask *const cpu_possible_mask = to_cpumask(cpu_possible_bits);


static DECLARE_BITMAP(cpu_online_bits, CONFIG_NR_CPUS) ;
const struct cpumask *const cpu_online_mask = to_cpumask(cpu_online_bits);


static DECLARE_BITMAP(cpu_present_bits, CONFIG_NR_CPUS) ;
const struct cpumask *const cpu_present_mask = to_cpumask(cpu_present_bits);


static DECLARE_BITMAP(cpu_active_bits, CONFIG_NR_CPUS) ;
const struct cpumask *const cpu_active_mask = to_cpumask(cpu_active_bits);


void set_cpu_possible(unsigned int cpu, bool possible)
{
	if (possible)
		cpumask_set_cpu(cpu, to_cpumask(cpu_possible_bits));
	else
		cpumask_clear_cpu(cpu, to_cpumask(cpu_possible_bits));
}

void set_cpu_present(unsigned int cpu, bool present)
{
	if (present)
		cpumask_set_cpu(cpu, to_cpumask(cpu_present_bits));
	else
		cpumask_clear_cpu(cpu, to_cpumask(cpu_present_bits));
}

void set_cpu_online(unsigned int cpu, bool online)
{
	if (online)
		cpumask_set_cpu(cpu, to_cpumask(cpu_online_bits));
	else
		cpumask_clear_cpu(cpu, to_cpumask(cpu_online_bits));
}

void set_cpu_active(unsigned int cpu, bool active)
{
	if (active)
		cpumask_set_cpu(cpu, to_cpumask(cpu_active_bits));
	else
		cpumask_clear_cpu(cpu, to_cpumask(cpu_active_bits));
}

void init_cpu_present(const struct cpumask *src)
{
	cpumask_copy(to_cpumask(cpu_present_bits), src);
}

void init_cpu_possible(const struct cpumask *src)
{
	cpumask_copy(to_cpumask(cpu_possible_bits), src);
}

void init_cpu_online(const struct cpumask *src)
{
	cpumask_copy(to_cpumask(cpu_online_bits), src);
}

