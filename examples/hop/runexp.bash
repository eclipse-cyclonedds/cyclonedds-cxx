# using default 10 topics, 128 bytes

# first arg: part of filename of output file
# second arg: if empty, no stagger; if non-empty: stagger by 1ms
# remaining args: passed on to source
run () {
    logname=$1 ; shift
    stagger=$1 ; shift

    # writing all this junk keeps my CPU awake (macOS, M1)
    bin/hop sink -j100000 0 128 &
    bin/hop source -j100000 0 128 & kpids=$!
    sleep 2
    for i in {0..1} ; do
        bin/mop sink -o mop-$logname-$i.txt &
    done
    for i in {0..1} ; do
        s=$((2 * i))
        bin/mop source -k$i ${stagger:+-s$s} "$@" & kpids="$kpids $!"
    done
    sleep 10
    kill -INT $kpids
    wait
}

run s0x0 ""        # both sources start at same time, no sleep between writes
run s0x1 ""  -x    # both sources start at same time, 100us sleep between writes
run s1x0 "s"       # source 1 starts 1ms after source 1, no sleep between writes
run s1x1 "s" -x    # source 1 starts 1ms after source 1, 100us sleep between writes
