#!/usr/bin/awk -f
NR > 1 { total += $2 }
END {
    printf "Total Gold Emissions: %.2f\n", total
}
