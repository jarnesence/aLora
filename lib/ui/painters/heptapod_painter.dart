import 'dart:math';
import 'package:flutter/material.dart';
import 'package:fast_noise/fast_noise.dart';
import '../../data/models/heptapod_spectrum.dart';
import '../../data/models/wave_layer.dart';
import '../../core/theme.dart';

class HeptapodPainter extends CustomPainter {
  final HeptapodSpectrum spectrum;
  final int seed;
  final SimplexNoise _noise;

  HeptapodPainter({
    required this.spectrum,
    this.seed = 1337,
  }) : _noise = SimplexNoise(seed: seed, frequency: 0.01);

  @override
  void paint(Canvas canvas, Size size) {
    final center = Offset(size.width / 2, size.height / 2);
    final baseRadius = min(size.width, size.height) / 3.0;

    final paint = Paint()
      ..color = AppTheme.accent.withOpacity(0.04)
      ..style = PaintingStyle.stroke
      ..strokeWidth = 1.0
      ..blendMode = BlendMode.srcOver
      ..maskFilter = const MaskFilter.blur(BlurStyle.normal, 1.0);

    // Generate the base structure points
    List<Offset> mainLoop = [];
    List<List<Offset>> tendrils = [];

    // A. The "Fluid" Skeleton
    const int steps = 360;

    double coreAmp = spectrum.coreLayer.fold(0.0, (sum, layer) => sum + layer.amplitude);
    double skeletonAmp = coreAmp * 30.0;
    if (skeletonAmp == 0) skeletonAmp = 10.0;

    for (int i = 0; i <= steps; i++) {
      double theta = (i / steps) * 2 * pi;

      double n1 = _noise.getNoise2(cos(theta) * 1.0, sin(theta) * 1.0);
      double n2 = _noise.getNoise2(cos(theta) * 3.0, sin(theta) * 3.0);

      double r = baseRadius + (skeletonAmp * (n1 + 0.5 * n2));

      double x = center.dx + r * cos(theta);
      double y = center.dy + r * sin(theta);
      mainLoop.add(Offset(x, y));
    }

    // B. The "Spidery" Tendrils
    // We look for angles where the spectrum (Nuance/Chaos) is active.
    // Instead of random angles, we can scan the circle.
    // But since chaos is a single factor per layer, we distribute them randomly or based on noise peaks.
    // Let's stick to the previous logic but improve placement.

    double maxChaos = spectrum.nuanceLayer.fold(0.0, (sum, layer) => max(sum, layer.chaosFactor));
    double highFreqAmp = spectrum.nuanceLayer.fold(0.0, (sum, layer) => sum + layer.amplitude);

    List<double> chaosAngles = [];

    if (maxChaos > 0.1 || highFreqAmp > 0.1) {
       int tendrilCount = (maxChaos * 10).toInt().clamp(3, 12);

       for (int t = 0; t < tendrilCount; t++) {
         // Use noise to find "peaks" for tendrils? Or just random around the circle.
         // Let's use random but consistent with seed.
         double angle = _randomRange(seed + t, 0, 2 * pi);
         chaosAngles.add(angle);

         double n1 = _noise.getNoise2(cos(angle) * 1.0, sin(angle) * 1.0);
         double n2 = _noise.getNoise2(cos(angle) * 3.0, sin(angle) * 3.0);
         double startR = baseRadius + (skeletonAmp * (n1 + 0.5 * n2));
         Offset startPoint = Offset(center.dx + startR * cos(angle), center.dy + startR * sin(angle));

         double length = baseRadius * (0.5 + maxChaos);
         // Tapering: we can't easily change stroke width in the stacked loop without performance hit.
         // But we can generate the path.
         _generateBranch(tendrils, startPoint, angle, length, 3, _noise, t);
       }
    }

    // C. The "Micro-Texture" Rendering (Stacking)
    for (int layerID = 0; layerID < 50; layerID++) {
      Path layerPath = Path();

      // Add Main Loop
      if (mainLoop.isNotEmpty) {
        List<Offset> noisyLoop = _offsetPoints(mainLoop, layerID);
        if (noisyLoop.isNotEmpty) {
          layerPath.moveTo(noisyLoop[0].dx, noisyLoop[0].dy);
          for (int k = 1; k < noisyLoop.length; k++) {
            layerPath.lineTo(noisyLoop[k].dx, noisyLoop[k].dy);
          }
          layerPath.close();
        }
      }

      // Add Tendrils
      for (var branch in tendrils) {
        if (branch.isEmpty) continue;
        List<Offset> noisyBranch = _offsetPoints(branch, layerID);
        if (noisyBranch.isNotEmpty) {
          layerPath.moveTo(noisyBranch[0].dx, noisyBranch[0].dy);
          for (int k = 1; k < noisyBranch.length; k++) {
            layerPath.lineTo(noisyBranch[k].dx, noisyBranch[k].dy);
          }
        }
      }

      canvas.drawPath(layerPath, paint);
    }

    // D. The "Splatter" Particle System
    final splatterPaint = Paint()
      ..color = AppTheme.accent.withOpacity(0.6)
      ..style = PaintingStyle.fill;

    int particleCount = 20 + Random(seed).nextInt(30);
    for (int i = 0; i < particleCount; i++) {
       // Cluster around chaos angles if any, otherwise random
       double angle;
       if (chaosAngles.isNotEmpty && i < particleCount * 0.7) {
         // 70% of particles near tendrils
         double baseAngle = chaosAngles[i % chaosAngles.length];
         angle = baseAngle + _randomRange(seed + i * 200, -0.3, 0.3); // Cluster width
       } else {
         angle = _randomRange(seed + i * 100, 0, 2 * pi);
       }

       double n = _noise.getNoise2(cos(angle), sin(angle));
       double r = baseRadius + (skeletonAmp * 2.0 * n) + _randomRange(i, -20, 50);

       double px = center.dx + r * cos(angle);
       double py = center.dy + r * sin(angle);

       canvas.drawCircle(Offset(px, py), 1.5, splatterPaint);
    }
  }

  List<Offset> _offsetPoints(List<Offset> points, int layerID) {
     return points.map((p) {
       double offsetX = _noise.getNoise2(p.dx * 0.02, layerID.toDouble() * 10.0) * 5.0;
       double offsetY = _noise.getNoise2(p.dy * 0.02, layerID.toDouble() * 10.0 + 1000) * 5.0;
       return p.translate(offsetX, offsetY);
     }).toList();
  }

  void _generateBranch(
    List<List<Offset>> distinctPaths,
    Offset start,
    double angle,
    double length,
    int depth,
    SimplexNoise noise,
    int branchId
  ) {
    if (depth <= 0) return;

    List<Offset> branchPoints = [];
    branchPoints.add(start);

    int segments = 20;
    double segmentLen = length / segments;

    Offset current = start;
    double currentAngle = angle;

    for (int i = 0; i < segments; i++) {
       double curl = noise.getNoise2(current.dx * 0.01, current.dy * 0.01) * 2.0;
       currentAngle += curl * 0.5;

       double nextX = current.dx + segmentLen * cos(currentAngle);
       double nextY = current.dy + segmentLen * sin(currentAngle);
       current = Offset(nextX, nextY);
       branchPoints.add(current);
    }
    distinctPaths.add(branchPoints);

    if (depth > 1) {
       _generateBranch(distinctPaths, current, currentAngle - 0.3, length * 0.6, depth - 1, noise, branchId + 1);
       _generateBranch(distinctPaths, current, currentAngle + 0.3, length * 0.6, depth - 1, noise, branchId + 2);
    }
  }

  double _randomRange(int seed, double min, double max) {
    var r = Random(seed);
    return min + r.nextDouble() * (max - min);
  }

  @override
  bool shouldRepaint(covariant HeptapodPainter oldDelegate) {
    return oldDelegate.spectrum != spectrum;
  }
}
