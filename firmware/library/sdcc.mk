ifeq (,$(SDCC_EXECUTABLE))
  ifeq (, $(shell command -v sdcc))
    ifeq (, $(shell command -v sdcc-sdcc))
      $(error "No sdcc executable found. Consider installing the sdcc package, or specifying an alternate executable with SDCC_EXECTUABLE")
    else
      SDCC_EXECUTABLE = sdcc-sdcc
    endif
  else
    SDCC_EXECUTABLE = sdcc
  endif
endif

ifeq (,$(SDAS_EXECUTABLE))
  ifeq (, $(shell command -v sdas8051))
    ifeq (, $(shell command -v sdcc-sdas8051))
      $(error "No sdas8051 executable found. Consider installing the sdcc package, or specifying an alternate executable with SDAS_EXECTUABLE")
    else
      SDAS_EXECUTABLE = sdcc-sdas8051
    endif
  else
    SDAS_EXECUTABLE = sdas8051
  endif
endif

ifeq (,$(SDAR_EXECUTABLE))
  ifeq (, $(shell command -v sdar))
    ifeq (, $(shell command -v sdcc-sdar))
      $(error "No sdar executable found. Consider installing the sdcc package, or specifying an alternate executable with SDAR_EXECTUABLE")
    else
      SDAR_EXECUTABLE = sdcc-sdar
    endif
  else
    SDAR_EXECUTABLE = sdar
  endif
endif
