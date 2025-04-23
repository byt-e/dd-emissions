#!/usr/bin/awk -f
NR > 1 {
    if ($5 > max) {
        max = $5
        date = $1
    }
}
END {
    printf "Max USD Value: %.2f on %s\n", max, date
}
