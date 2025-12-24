class WaveLayer {
  final double frequency;
  final double amplitude;
  final String shapeType;
  final double chaosFactor;

  WaveLayer({
    required this.frequency,
    required this.amplitude,
    this.shapeType = 'circle',
    this.chaosFactor = 0.0,
  });

  factory WaveLayer.fromJson(Map<String, dynamic> json) {
    return WaveLayer(
      frequency: (json['freq'] as num).toDouble(),
      amplitude: (json['amp'] as num).toDouble(),
      shapeType: json['shapeType'] as String? ?? 'circle',
      chaosFactor: (json['chaosFactor'] as num?)?.toDouble() ?? 0.0,
    );
  }

  @override
  String toString() {
    return 'WaveLayer(frequency: $frequency, amplitude: $amplitude, shapeType: $shapeType, chaosFactor: $chaosFactor)';
  }
}
