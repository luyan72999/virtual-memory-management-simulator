import matplotlib.pyplot as plt
import numpy as np


import re
import statistics


# Function to extract TLB hit rates from a text file
def extract_tlb_hit_rates(file_path):
    # Read the contents of the file
    with open(file_path, 'r') as file:
        file_contents = file.read()

    # Use regular expression to find all TLB hit rates
    tlb_hit_rate_pattern = r'TLB hit rate: (\d+\.\d+)'
    matches = re.findall(tlb_hit_rate_pattern, file_contents)

    # Check if matches are found
    if matches:
        # Convert the extracted values to floats
        tlb_hit_rates = [float(match) for match in matches]

        rates_with_20 = tlb_hit_rates[:10]
        rates_with_50 = tlb_hit_rates[10:20]
        rates_with_90 = tlb_hit_rates[20:30]

        if rates_with_20:
            average_rates_with_20 = statistics.mean(rates_with_20)
        if rates_with_50:
            average_rates_with_50 = statistics.mean(rates_with_50)
        if rates_with_90:
            average_rates_with_90 = statistics.mean(rates_with_90)

        return average_rates_with_20, average_rates_with_50, average_rates_with_90
    else:
        print("TLB hit rates not found in the file.")
        return None, None,None


# case 1: one-level TLB and 4KB page size
file_path = 'results_case1.txt'
rates_with_20_case1, rates_with_50_case1,rates_with_90_case1 = extract_tlb_hit_rates(file_path)

# case 2: one-level TLB and variable page sizes
file_path = 'results_case2.txt'
rates_with_20_case2, rates_with_50_case2,rates_with_90_case2 = extract_tlb_hit_rates(file_path)

# case 3: two-level TLB and 4KB page size
file_path = 'results_case3.txt'
rates_with_20_case3, rates_with_50_case3,rates_with_90_case3 = extract_tlb_hit_rates(file_path)

# case 4: two-level TLB and variable page sizes
file_path = 'results_case4.txt'
rates_with_20_case4, rates_with_50_case4,rates_with_90_case4 = extract_tlb_hit_rates(file_path)


x = ["0.2","0.5","0.9"]
case1 = [rates_with_20_case1, rates_with_50_case1,rates_with_90_case1]
case2 = [rates_with_20_case2, rates_with_50_case2,rates_with_90_case2]
case3 = [rates_with_20_case3, rates_with_50_case3,rates_with_90_case3]
case4 = [rates_with_20_case4, rates_with_50_case4,rates_with_90_case4]

indices = np.arange(len(x))

# figure with 4 cases
bar_width = 0.2

plt.figure(figsize=(10, 5))
plt.bar(indices - 1.5*bar_width, case1, bar_width, color='skyblue', label='one level TLB and 4KB page size')
plt.bar(indices - 0.5*bar_width, case2, bar_width, color='slateblue', label='one level TLB and variable page sizes')
plt.bar(indices + 0.5*bar_width, case3, bar_width, color='gray', label='two level TLB and 4KB page size')
plt.bar(indices + 1.5*bar_width, case4, bar_width, color='orange', label='two level TLB and variable page sizes')


plt.xlabel('Locality')
plt.ylabel('TLB hit rate')
# plt.title('Two-level TLB and Only Support 4KB Page Size')

plt.xticks(indices,x)
plt.legend()
plt.ylim(0.7, plt.ylim()[1])

plt.savefig('plot4cases.png', dpi=300)


# figure showing 4kb/variable, based on two level TLB
bar_width = 0.4
plt.figure(figsize=(8, 5))
plt.bar(indices - 0.5*bar_width, case3, bar_width, color='gray', label='4KB page size')
plt.bar(indices + 0.5*bar_width, case4, bar_width, color='orange', label='variable page sizes')


plt.xlabel('Locality')
plt.ylabel('TLB hit rate')
plt.title('Supporting Variable Page Sizes VS Only 4KB Page Size')

plt.xticks(indices,x)
plt.legend(loc='upper left')
plt.ylim(0.7, plt.ylim()[1])

plt.savefig('plot_compare_pageSize.png', dpi=300)

# figure showing one/two level TLB, based on variable page sizes
bar_width = 0.4
plt.figure(figsize=(8, 5))
plt.bar(indices - 0.5*bar_width, case2, bar_width, color='slateblue', label='one level TLB')
plt.bar(indices + 0.5*bar_width, case4, bar_width, color='orange', label='two level TLB')


plt.xlabel('Locality')
plt.ylabel('TLB hit rate')
plt.title('One-Level TLB VS Two-Level TLB')

plt.xticks(indices,x)
plt.legend(loc='upper left')
plt.ylim(0.7, plt.ylim()[1])

plt.savefig('plot_compare_TLB.png', dpi=300)