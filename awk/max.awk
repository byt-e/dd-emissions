#!/usr/bin/awk -f
NR > 1 && $2 > max {
    max = $2
    date = $1
}
END {
    printf "Max Emissions: %.2f on %s\n", max, date
}
