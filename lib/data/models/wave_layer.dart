class WaveLayer {
  final double frequency;
  final double amplitude;
  final String shapeType;

  WaveLayer({
    required this.frequency,
    required this.amplitude,
    required this.shapeType,
  });

  factory WaveLayer.fromJson(Map<String, dynamic> json) {
    return WaveLayer(
      frequency: (json['freq'] as num).toDouble(),
      amplitude: (json['amp'] as num).toDouble(),
      shapeType: json['shape'] as String,
    );
  }

  Map<String, dynamic> toJson() {
    return {
      'freq': frequency,
      'amp': amplitude,
      'shape': shapeType,
    };
  }
}
