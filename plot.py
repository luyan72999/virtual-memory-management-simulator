import matplotlib.pyplot as plt

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


# Example usage
file_path = 'results.txt'  # Replace with the actual file path
rates_with_20, rates_with_50,rates_with_90 = extract_tlb_hit_rates(file_path)


x = ["0.2","0.5","0.9"]
y = [rates_with_20, rates_with_50,rates_with_90]

bar_width = 1
plt.bar(x, y, color='orange')

plt.xlabel('Locality')
plt.ylabel('TLB hit rate')
plt.title('Two-level TLB and Only Support 4KB Page Size')

plt.xticks(x)


plt.savefig('Two_TLB_4KB.png', dpi=300)
