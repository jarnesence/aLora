import 'wave_layer.dart';

class HeptapodSpectrum {
  final List<WaveLayer> coreLayer;
  final List<WaveLayer> narrativeLayer;
  final List<WaveLayer> nuanceLayer;

  HeptapodSpectrum({
    required this.coreLayer,
    required this.narrativeLayer,
    required this.nuanceLayer,
  });

  factory HeptapodSpectrum.fromJson(Map<String, dynamic> json) {
    return HeptapodSpectrum(
      coreLayer: (json['core'] as List)
          .map((e) => WaveLayer.fromJson(e as Map<String, dynamic>))
          .toList(),
      narrativeLayer: (json['narrative'] as List)
          .map((e) => WaveLayer.fromJson(e as Map<String, dynamic>))
          .toList(),
      nuanceLayer: (json['nuance'] as List)
          .map((e) => WaveLayer.fromJson(e as Map<String, dynamic>))
          .toList(),
    );
  }

  Map<String, dynamic> toJson() {
    return {
      'core': coreLayer.map((e) => e.toJson()).toList(),
      'narrative': narrativeLayer.map((e) => e.toJson()).toList(),
      'nuance': nuanceLayer.map((e) => e.toJson()).toList(),
    };
  }
}
