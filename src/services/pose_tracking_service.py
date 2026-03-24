from __future__ import annotations

from src.models.contracts import FramePacket, PoseResult


class PoseTrackingService:
    def process_frame(self, frame_packet: FramePacket) -> PoseResult:
        return PoseResult(frame_id=frame_packet.frame_id)
