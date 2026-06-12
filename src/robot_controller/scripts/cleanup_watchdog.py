#!/usr/bin/env python3
"""
Watchdog: monitors gzserver and rviz2 processes.
When both exit, waits GRACE_PERIOD seconds, then gracefully terminates remaining ROS processes.

Usage: cleanup_watchdog.py [grace_period_seconds]
Default grace period: 10 seconds (override via GRACE_PERIOD env var or argv)
"""

import subprocess
import time
import os
import sys
import signal

GRACE_PERIOD = int(os.environ.get("GRACE_PERIOD", "10"))
if len(sys.argv) > 1:
    GRACE_PERIOD = int(sys.argv[1])

POLL_INTERVAL = 2
TERM_WAIT = 5
KILL_WAIT = 3

PROCESS_NAMES = [
    "gzserver",
    "rviz2",
    "gzclient",
    "component_container",
    "robot_state_publisher",
    "spawn_entity",
    "visual_follower",
    "mode_manager",
    "nav2",
]


def process_running(name):
    """检查进程是否在运行"""
    try:
        result = subprocess.run(
            ["pgrep", "-f", name],
            capture_output=True,
            timeout=5,
        )
        return result.returncode == 0
    except (subprocess.TimeoutExpired, FileNotFoundError):
        return False


def get_pids(name):
    """获取进程的PID列表"""
    try:
        result = subprocess.run(
            ["pgrep", "-f", name],
            capture_output=True,
            text=True,
            timeout=5,
        )
        if result.returncode == 0:
            return [int(pid) for pid in result.stdout.strip().split()]
        return []
    except (subprocess.TimeoutExpired, FileNotFoundError):
        return []


def graceful_terminate_process(name):
    """温和终止进程：先 SIGTERM，等待后如果还在则 SIGKILL"""
    pids = get_pids(name)
    if not pids:
        return
    
    print(f"  [watchdog] Terminating {name} (PIDs: {pids})...")
    
    for pid in pids:
        try:
            os.kill(pid, signal.SIGTERM)
        except ProcessLookupError:
            pass
        except PermissionError:
            print(f"    [warning] No permission to terminate PID {pid}")
    
    time.sleep(TERM_WAIT)
    
    remaining_pids = get_pids(name)
    if remaining_pids:
        print(f"  [watchdog] {name} didn't exit gracefully, forcing...")
        for pid in remaining_pids:
            try:
                os.kill(pid, signal.SIGKILL)
            except ProcessLookupError:
                pass
            except PermissionError:
                print(f"    [warning] No permission to kill PID {pid}")
        time.sleep(KILL_WAIT)


def kill_ros2_launch():
    """温和终止 ros2 launch 进程树"""
    try:
        result = subprocess.run(
            ["pgrep", "-f", "ros2 launch"],
            capture_output=True,
            text=True,
            timeout=5,
        )
        
        if result.returncode == 0:
            pids = [int(pid) for pid in result.stdout.strip().split()]
            print(f"  [watchdog] Terminating ros2 launch processes (PIDs: {pids})...")
            
            for pid in pids:
                try:
                    os.kill(pid, signal.SIGINT)
                except ProcessLookupError:
                    pass
                except PermissionError:
                    print(f"    [warning] No permission to interrupt PID {pid}")
            
            time.sleep(TERM_WAIT)
            
            remaining = subprocess.run(
                ["pgrep", "-f", "ros2 launch"],
                capture_output=True,
                timeout=5,
            )
            if remaining.returncode == 0:
                print(f"  [watchdog] Some ros2 launch processes still running, forcing...")
                subprocess.run(
                    ["pkill", "-9", "-f", "ros2 launch"],
                    capture_output=True,
                    timeout=5,
                )
    except subprocess.TimeoutExpired:
        pass


def cleanup_all():
    """温和清理所有进程"""
    print(f"[watchdog] Starting graceful cleanup...")
    
    for proc in ["gzclient", "rviz2"]:
        if process_running(proc):
            graceful_terminate_process(proc)
    
    for proc in ["nav2", "visual_follower", "mode_manager"]:
        if process_running(proc):
            graceful_terminate_process(proc)
    
    for proc in ["component_container", "robot_state_publisher", "spawn_entity"]:
        if process_running(proc):
            graceful_terminate_process(proc)
    
    if process_running("gzserver"):
        graceful_terminate_process("gzserver")
    
    kill_ros2_launch()
    
    print(f"[watchdog] Cleanup complete.")


def main():
    print(f"[watchdog] Monitoring gzserver + rviz2...")
    print(f"[watchdog] Grace period: {GRACE_PERIOD}s, poll interval: {POLL_INTERVAL}s")
    print(f"[watchdog] Using graceful termination (SIGTERM, wait {TERM_WAIT}s, then SIGKILL if needed)")

    gz_gone = False
    rviz_gone = False

    try:
        while not (gz_gone and rviz_gone):
            time.sleep(POLL_INTERVAL)

            if not gz_gone and not process_running("gzserver"):
                print(f"[watchdog] gzserver exited")
                gz_gone = True

            if not rviz_gone and not process_running("rviz2"):
                print(f"[watchdog] rviz2 exited")
                rviz_gone = True

        print(f"[watchdog] Both gzserver and rviz2 have exited.")
        print(f"[watchdog] Waiting {GRACE_PERIOD}s before cleanup...")
        time.sleep(GRACE_PERIOD)

        cleanup_all()

    except KeyboardInterrupt:
        print(f"[watchdog] Interrupted, cleaning up now...")
        cleanup_all()


if __name__ == "__main__":
    main()
