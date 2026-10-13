package main

import "testing"

func TestPlainNumbers(t *testing.T) {
	cases := map[string]string{
		`{"wet":1e-05,"h":0.4866,"q":-1,"n":"Apple 1e-05","w":4.2E+03,"z":0}`: `{"wet":0.00001,"h":0.4866,"q":-1,"n":"Apple 1e-05","w":4200,"z":0}`,
		`{"a":[1.5e-7,-2e2],"s":"x\"1e5\""}`:                                    `{"a":[0.00000015,-200],"s":"x\"1e5\""}`,
		`{"t":5.}`:                                                               `{"t":5.0}`,
		`{"keep":"order","b":2,"a":1}`:                                           `{"keep":"order","b":2,"a":1}`,
	}
	for in, want := range cases {
		if got := plainNumbers(in); got != want {
			t.Errorf("plainNumbers(%s)\n got %s\nwant %s", in, got, want)
		}
	}
}
