#!/usr/bin/awk -f
NR > 1 {
    values[NR] = $5
    total += $5
    count++
}
END {
    if (count < 2) {
        print "Not enough data for volatility calculation."
        exit
    }

    avg = total / count

    for (i = 2; i <= count + 1; i++) {
        sumsq += (values[i] - avg) ^ 2
    }

    stdev = sqrt(sumsq / (count - 1))
    printf "Price Volatility (stdev): %.4f\n", stdev
}
