#!/usr/bin/awk -f
NR > 1 {
    total += $5
    count++
}
END {
    if (count > 0)
        printf "Average USD Value: %.2f\n", total/count
    else
        print "No data to compute average"
}
