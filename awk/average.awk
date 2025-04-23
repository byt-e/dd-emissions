#!/usr/bin/awk -f
NR > 1 {
    total += $2
    count ++
}
END {
    if (count > 0)
        printf "Average Gold Emission: %.2f\n", total / count
    else
        print "No data."
}
