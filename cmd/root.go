package cmd

import (
	"fmt"
	"os"

	"github.com/spf13/cobra"
	"github.com/spf13/viper"
)

var cfgFile string

var rootCmd = &cobra.Command{
	Use:   "systemmonitor",
	Short: "System metrics monitor for ESP32 displays",
	Long: `Pulse Monitor - Sends real-time system metrics (CPU, Memory, GPU, Network, Disk)
to an ESP32-based display via serial connection. Includes systray interface for
easy theme and accent color customization.`,
	Run: func(cmd *cobra.Command, args []string) {
		// Run with systray GUI by default
		runWithSystray()
	},
}

func Execute() {
	err := rootCmd.Execute()
	if err != nil {
		os.Exit(1)
	}
}

func init() {
	cobra.OnInitialize(initConfig)

	rootCmd.PersistentFlags().StringVar(&cfgFile, "config", "", "config file (default is $HOME/.systemmonitor.yaml)")
}

func initConfig() {
	if cfgFile != "" {
		viper.SetConfigFile(cfgFile)
	} else {
		home, err := os.UserHomeDir()
		cobra.CheckErr(err)

		viper.AddConfigPath(home)
		viper.SetConfigType("yaml")
		viper.SetConfigName(".systemmonitor")
	}

	// Bind environment variables
	viper.SetEnvPrefix("PULSE")
	viper.AutomaticEnv()

	// Set defaults
	viper.SetDefault("theme", "light")
	viper.SetDefault("accent", "sapphire")

	if err := viper.ReadInConfig(); err == nil {
		fmt.Fprintln(os.Stderr, "Using config file:", viper.ConfigFileUsed())
	}
}
