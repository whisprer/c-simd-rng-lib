rng128 = wofl's rust port of xoroshiro128+ via C++ RNG ver
StdRng = rust std rng from random
SmallRng = rust rng implementation employing xoroshiro128++


1000000000 rng128 calls
        1.2980007 sec
        1.339999 sec
        1.2938669 sec
        1.5100090000000002 sec
        1.2893676 sec
        1.3109428 sec
        1.3751248999999999 sec
        1.3155885 sec
        1.2851015000000001 sec
        1.28515 sec

1000000000 StdRng calls
        4.7556079 sec
        4.2760701 sec
        4.2629807 sec
        4.1731993 sec
        4.2279873 sec
        4.1941022 sec
        4.2451411 sec
        4.2875073 sec
        4.3213745 sec
        4.1676789 sec

1000000000 SmallRng calls
        1.5126949 sec
        1.5327762 sec
        1.5377158 sec
        1.6621743 sec
        1.5949436000000001 sec
        1.6801401999999999 sec
        1.529946 sec
        1.65436 sec
        1.4968408 sec
        1.5464419 sec

example seed gen times:
100000 tsc_seed calls
        0.4536303 sec
100000 random_device_seed calls
        0.0112179 sec



     Running unittests src\main.rs (target\release\deps\rng-1eab8e0c9a0bb5cf.exe)

running 0 tests

test result: ok. 0 passed; 0 failed; 0 ignored; 0 measured; 0 filtered out; finished in 0.00s

     Running benches\benchmark.rs (target\release\deps\benchmark-b7740e4fba054652.exe)
WARNING: HTML report generation will become a non-default optional feature in Criterion.rs 0.4.0.
This feature is being moved to cargo-criterion (https://github.com/bheisler/cargo-criterion) and will be optional in a future version of Criterion.rs. To silence this warning, either switch to cargo-criterion or enable the 'html_reports' feature in your Cargo.toml.

Gnuplot not found, using plotters backend
rng128                  time:   [1.4614 ns 1.4968 ns 1.5351 ns]
                        change: [+0.6473% +2.5521% +4.3813%] (p = 0.01 < 0.05)
                        Change within noise threshold.
Found 12 outliers among 100 measurements (12.00%)
  7 (7.00%) high mild
  5 (5.00%) high severe

StdRng                  time:   [4.1027 ns 4.1436 ns 4.1922 ns]
                        change: [-10.438% -7.5633% -4.4601%] (p = 0.00 < 0.05)
                        Performance has improved.
Found 12 outliers among 100 measurements (12.00%)
  8 (8.00%) high mild
  4 (4.00%) high severe

SmallRng                time:   [1.4978 ns 1.6072 ns 1.7268 ns]
                        change: [+13.148% +18.792% +25.076%] (p = 0.00 < 0.05)
                        Performance has regressed.
Found 15 outliers among 100 measurements (15.00%)
  5 (5.00%) high mild
  10 (10.00%) high severe

PS C:\rust_projects\rng>