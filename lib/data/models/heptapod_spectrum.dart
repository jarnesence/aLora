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
    var core = json['core'] as List? ?? [];
    var narrative = json['narrative'] as List? ?? [];
    var nuance = json['nuance'] as List? ?? [];

    return HeptapodSpectrum(
      coreLayer: core.map((e) => WaveLayer.fromJson(e as Map<String, dynamic>)).toList(),
      narrativeLayer: narrative.map((e) => WaveLayer.fromJson(e as Map<String, dynamic>)).toList(),
      nuanceLayer: nuance.map((e) => WaveLayer.fromJson(e as Map<String, dynamic>)).toList(),
    );
  }

  @override
  String toString() {
    return 'HeptapodSpectrum(coreLayer: $coreLayer, narrativeLayer: $narrativeLayer, nuanceLayer: $nuanceLayer)';
  }
}
