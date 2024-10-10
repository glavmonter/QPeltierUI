class IIR(object):
    def __init__(self, acoeff: list[float], bcoeff: list[float]) -> None:
        self._empty = False
        if len(acoeff) == 0 and len(bcoeff) == 0:
            self._empty = True
            return
        
        if len(acoeff) != len(bcoeff):
            raise ValueError('The size of acoeff must be equal to the bcoeff')
        
        self._acoeff = acoeff
        self._bcoeff = bcoeff
        self._ncoeff = len(acoeff) - 1
        if self._ncoeff <= 2:
            raise ValueError('The sizeof of coefficients must be greate than 2')
        self._x = [0.0] * (self._ncoeff + 1)
        self._y = [0.0] * (self._ncoeff + 1)
    
    def process(self, value: float) -> float:
        if self._empty:
            return value
        
        for n in range(self._ncoeff, 0, -1):
            self._x[n] = self._x[n - 1]
            self._y[n] = self._y[n - 1]
        
        self._x[0] = value
        self._y[0] = self._acoeff[0] * self._x[0]
        for n in range(1, self._ncoeff + 1, 1):
            self._y[0] += self._acoeff[n] * self._x[n] - self._bcoeff[n] * self._y[n]
        return self._y[0]
