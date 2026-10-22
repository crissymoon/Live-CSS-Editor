// Command gateway runs the authenticated chat proxy that fronts one or more
// model servers. It shares the xcm_auth access-token secret, so a token minted
// by the auth service works here without a second login.
package main

import (
	"context"
	"log"
	"os"
	"os/signal"
	"syscall"
	"time"

	"xcaliburmoon.net/xcm_auth/gateway"
)

func main() {
	cfg, err := gateway.LoadConfig()
	if err != nil {
		log.Fatalf("[gateway] config: %v", err)
	}

	srv := gateway.New(cfg)

	quit := make(chan os.Signal, 1)
	signal.Notify(quit, syscall.SIGINT, syscall.SIGTERM)
	go func() {
		<-quit
		log.Println("[gateway] shutdown signal received")
		ctx, cancel := context.WithTimeout(context.Background(), 10*time.Second)
		defer cancel()
		if err := srv.Shutdown(ctx); err != nil {
			log.Printf("[gateway] shutdown error: %v", err)
		}
	}()

	if err := srv.Start(); err != nil {
		log.Printf("[gateway] stopped: %v", err)
	}
}
