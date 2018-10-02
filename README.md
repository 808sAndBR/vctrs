
<!-- README.md is generated from README.Rmd. Please edit that file -->

# vctrs <img src="man/figures/logo.png" align="right" width=120 height=139 alt="" />

[![Travis build
status](https://travis-ci.org/r-lib/vctrs.svg?branch=master)](https://travis-ci.org/r-lib/vctrs)
[![Coverage
status](https://codecov.io/gh/r-lib/vctrs/branch/master/graph/badge.svg)](https://codecov.io/github/r-lib/vctrs?branch=master)
[![lifecycle](https://img.shields.io/badge/lifecycle-experimental-orange.svg)](https://www.tidyverse.org/lifecycle/#experimental)

The vctrs packages provides developer tools for working with existing
vector classes (including logical, integer, double, character, list,
factor, ordered, Date, POSIXct, difftime, and data.frame) as well as
creating new vctors classes. It has two related goals:

1.  Through the `vec_type2()` and `vec_cast()` generics, define a
    consistent set of principles that determine the return type of
    functions that have multiple input types (e.g. `c()`, `rbind()`,
    `ifelse()`).
    
    See `vignette("function-types")` for more details.

2.  Through `new_vctr()` and `new_rcrd()`, make it easy to create new S3
    vector classes whose behaviour can be defined my implementing just a
    handful of key methods.
    
    See `vignette("s3-vector")` for more details.

vctrs is a developer focused package. Understanding and extending vctrs
requires some effort from developers, but should be invisible to most
users. It’s our hope that having an underlying theory will mean that
users can build up an accurate mental model without explicitly learning
the theory. vctrs will typically be used by other packages, making it
easy for them to provide new classes of S3 vectors that are supported
throughout the tidyverse (and beyond). For that reason, vctrs has few
dependencies.

## Installation

vctrs is not currently on CRAN. Install the development version from
GitHub with:

``` r
# install.packages("devtools")
devtools::install_github("r-lib/vctrs")
```

## Usage

``` r
library(vctrs)

vec_c(factor("a"), factor("b"))
#> [1] a b
#> Levels: a b
vec_c(Sys.Date(), Sys.time())
#> [1] "2018-10-02 00:00:00 CDT" "2018-10-02 16:35:50 CDT"
```
