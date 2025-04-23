#!/usr/bin/awk -f

BEGIN {
    user_initial = -1  # default: not provided
}

NR > 1 {
    if (NR == 2 && user_initial < 0) {
        initial = $5
    } else if (NR == 2 && user_initial >= 0) {
        initial = user_initial
    }
    final = $5
}

END {
    if (initial == 0) {
        print "Initial investment is zero; cannot compute ROI."
        exit 1
    }

    roi = ((final - initial) / initial) * 100
    printf "ROI: %.2f%% (Initial: %.2f → Final: %.2f)\n", roi, initial, final
}

