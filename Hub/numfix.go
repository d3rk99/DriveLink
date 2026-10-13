package main

import (
	"strconv"
	"strings"
)

// plainNumbers rewrites every JSON number written in exponent form (1e-05, 4.2E+03) as a plain decimal
// (0.00001, 4200) and leaves everything else - key order, strings, spacing - byte for byte as it was.
//
// Why (M1 test 2026-10-13, 2f8669a): DayZ's own JsonSerializer wrote the player snapshot, but reading that exact
// text back failed with "Miss fraction part in number". The same snapshot passed through PostgreSQL JSONB (which
// rewrites numbers in plain form) had parsed fine the run before. So DayZ writes exponent numbers it cannot read.
func plainNumbers(s string) string {
	var b strings.Builder
	b.Grow(len(s))
	inStr, esc := false, false
	for i := 0; i < len(s); {
		c := s[i]
		if inStr {
			b.WriteByte(c)
			switch {
			case esc:
				esc = false
			case c == '\\':
				esc = true
			case c == '"':
				inStr = false
			}
			i++
			continue
		}
		if c == '"' {
			inStr = true
			b.WriteByte(c)
			i++
			continue
		}
		if c == '-' || (c >= '0' && c <= '9') {
			j := i + 1
			for j < len(s) && strings.IndexByte("0123456789.eE+-", s[j]) >= 0 {
				j++
			}
			num := s[i:j]
			if strings.ContainsAny(num, "eE") {
				if f, err := strconv.ParseFloat(num, 64); err == nil {
					num = strconv.FormatFloat(f, 'f', -1, 64)
				}
			}
			// "5." or ".5" style would also break DayZ: make sure a dot has digits on both sides
			if strings.HasSuffix(num, ".") {
				num += "0"
			}
			b.WriteString(num)
			i = j
			continue
		}
		b.WriteByte(c)
		i++
	}
	return b.String()
}
