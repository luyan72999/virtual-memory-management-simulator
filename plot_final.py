import matplotlib.pyplot as plt
import matplotlib.ticker as ticker
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

        rates_with_20_1 = tlb_hit_rates[:10]
        rates_with_20_4 = tlb_hit_rates[10:20]
        rates_with_20_8 = tlb_hit_rates[20:30]
        rates_with_50_1 = tlb_hit_rates[30:40]
        rates_with_50_4 = tlb_hit_rates[40:50]
        rates_with_50_8 = tlb_hit_rates[50:60]
        rates_with_90_1 = tlb_hit_rates[60:70]
        rates_with_90_4 = tlb_hit_rates[70:80]
        rates_with_90_8 = tlb_hit_rates[80:90]

        if rates_with_20_1:
            average_rates_with_20_1 = statistics.mean(rates_with_20_1)
        if rates_with_20_4:
            average_rates_with_20_4 = statistics.mean(rates_with_20_4)
        if rates_with_20_8:
            average_rates_with_20_8 = statistics.mean(rates_with_20_8)
        if rates_with_50_1:
            average_rates_with_50_1 = statistics.mean(rates_with_50_1)
        if rates_with_50_4:
            average_rates_with_50_4 = statistics.mean(rates_with_50_4)
        if rates_with_50_8:
            average_rates_with_50_8 = statistics.mean(rates_with_50_8)
        if rates_with_90_1:
            average_rates_with_90_1 = statistics.mean(rates_with_90_1)
        if rates_with_90_4:
            average_rates_with_90_4 = statistics.mean(rates_with_90_4)
        if rates_with_90_8:
            average_rates_with_90_8 = statistics.mean(rates_with_90_8)


        return average_rates_with_20_1, average_rates_with_20_4, average_rates_with_20_8, average_rates_with_50_1, average_rates_with_50_4, average_rates_with_50_8, average_rates_with_90_1, average_rates_with_90_4, average_rates_with_90_8
    else:
        print("TLB hit rates not found in the file.")
        return None, None,None,None,None,None,None,None,None


# # case 1: one-level TLB and 4KB page size
# file_path = 'results_case1.txt'
# rates_with_20_case1, rates_with_50_case1,rates_with_90_case1 = extract_tlb_hit_rates(file_path)
#
# # case 2: one-level TLB and variable page sizes
# file_path = 'results_case2.txt'
# rates_with_20_case2, rates_with_50_case2,rates_with_90_case2 = extract_tlb_hit_rates(file_path)
#
# # case 3: two-level TLB, random policy, 4KB page size
file_path = './results/results_case3.txt'
rates_with_20_1_case3, rates_with_20_4_case3,rates_with_20_8_case3, rates_with_50_1_case3,rates_with_50_4_case3,rates_with_50_8_case3,rates_with_90_1_case3,rates_with_90_4_case3,rates_with_90_8_case3 = extract_tlb_hit_rates(file_path)

# case 4: two-level TLB, random policy, variable page sizes
file_path = './results/results_case4.txt'
rates_with_20_1_case4, rates_with_20_4_case4,rates_with_20_8_case4, rates_with_50_1_case4,rates_with_50_4_case4,rates_with_50_8_case4,rates_with_90_1_case4,rates_with_90_4_case4,rates_with_90_8_case4 = extract_tlb_hit_rates(file_path)


x = ["0.2","0.5","0.9"]
y1 = [rates_with_20_1_case3, rates_with_50_1_case3,rates_with_90_1_case3]
y2 = [rates_with_20_1_case4, rates_with_50_1_case4,rates_with_90_1_case4]
y3 = [rates_with_20_4_case3, rates_with_50_4_case3,rates_with_90_4_case3]
y4 = [rates_with_20_4_case4, rates_with_50_4_case4,rates_with_90_4_case4]
y5 = [rates_with_20_8_case3, rates_with_50_8_case3,rates_with_90_8_case3]
y6 = [rates_with_20_8_case4, rates_with_50_8_case4,rates_with_90_8_case4]

fig, axs = plt.subplots(1, 3, sharey=True, figsize=(12, 4))

axs[0].plot(x, y1, label='4kb', color='blue')
axs[0].plot(x, y2, label='variable', color='orange')
axs[0].set_title('1 process')
axs[0].legend()

axs[1].plot(x, y3, label='4kb', color='blue')
axs[1].plot(x, y4, label='variable', color='orange')
axs[1].set_title('4 processes')
axs[1].legend()

axs[2].plot(x, y1, label='4kb', color='blue')
axs[2].plot(x, y2, label='variable', color='orange')
axs[2].set_title('8 processes')
axs[2].legend()

fig.text(0.5, 0.02, 'Locality', ha='center', va='center')
fig.text(0.008, 0.5, 'TLB hit rate', ha='center', va='center', rotation='vertical')

plt.tight_layout()
plt.savefig('plot_compare_pageSize.png', dpi=300)