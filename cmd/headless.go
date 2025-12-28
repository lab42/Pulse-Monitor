package cmd

import (
	"github.com/spf13/cobra"
)

var headlessCmd = &cobra.Command{
	Use:   "headless",
	Short: "Run without GUI (for servers/systemd)",
	Long: `Run Pulse Monitor in headless mode without the system tray interface.
Useful for servers, Docker containers, or systemd services that start before login.

Theme and accent can be controlled via environment variables or config file:
  PULSE_THEME=dark
  PULSE_ACCENT=mauve

Or in ~/.systemmonitor.yaml:
  theme: dark
  accent: mauve`,
	Run: func(cmd *cobra.Command, args []string) {
		runHeadless()
	},
}

func init() {
	rootCmd.AddCommand(headlessCmd)
}
