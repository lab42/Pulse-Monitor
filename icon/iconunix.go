//go:build linux || darwin

package icon

import (
	_ "embed"
)

//go:embed icon.svg
var Data []byte
