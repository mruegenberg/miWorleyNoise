{-# LANGUAGE BangPatterns #-}
module Main where

import Codec.BMP
import qualified Data.ByteString as BS
import Data.Word(Word8)
import Control.Monad
import Data.Function
import Data.List(sortBy,maximumBy,foldl')
import Data.Function(on)
import Random.MWC.Pure(seed, range_random, random_list)
import Data.Hashable
import Data.Map(Map)
import qualified Data.Map as Map
import Data.Maybe(isJust,fromJust)

-- Code based on Steve Worley's paper "A Cellular Texture Basis Function", and some material from Advanced Renderman (particularly the thresholding, chapter 10).

-- for memoization, to reduce need for random number generation. 
data Cache = Cache { cacheCube :: Cube, cacheVals :: Map Cube [Point] }

emptyCube = [-1,-1]

-- Helper functions
  
greyToRGBA :: Word8 -> [Word8]
greyToRGBA val = [val,val,val,255]

greyToColor :: [Word8] -> [Word8] -> Double -> [Word8]
greyToColor c1 c2 s = zipWith (\comp1 comp2 -> min 255 $ (round ((fromIntegral comp1) * (1 - s) + (fromIntegral comp2) * s))) c1 c2

type Point = [Int]
type Cube = Point

-- Distance metrics

linearDistance :: Point -> Point -> Double
linearDistance p1 p2 = sqrt (linearSqDistance p1 p2)

linearSqDistance :: Point -> Point -> Double
linearSqDistance p1 p2 = fromIntegral $ sum $ zipWith (\c1 c2 -> (c1 - c2)^2) p1 p2
        
manhattanDistance :: Point -> Point -> Double
manhattanDistance p1 p2 = sum $ map (abs . fromIntegral) $ zipWith (-) p1 p2

minkowskiDistance :: Double -> Point -> Point -> Double
minkowskiDistance p p1 p2 = (sum (map ((** p) . abs) diffs))**(1/p)
  where diffs = zipWith (\c1 c2 -> fromIntegral (c1 - c2)) p1 p2
        
-- distance = linearDistance
-- distScale = 35

distance = linearSqDistance
distScale = 160000

-- distance = manhattanDistance
-- distScale = 40

-- distance = minkowskiDistance 2
-- distScale = 30

-- Parameters

data VoronoiParams = VoronoiParams {
  color1 :: [Word8], -- color at center of cells
  color2 :: [Word8], -- color border between cells
  color3 :: Maybe [Word8] -- color for space between cells
  }
                     
-- distance between cubes.
cubeDist = 800

cubeRadius :: Double
cubeRadius = distance [0,0] [cubeDist `div` 2, cubeDist `div` 2]

-- compute the cube in which a point lies
pointCube :: Point -> Cube
pointCube p = map ((* cubeDist) . (`div` cubeDist)) p

-- coordinates for all cubes adjacent to a cube
neighborCubes :: Cube -> [Cube]
neighborCubes cube = tail $ combinations coords -- tail should filter out the cube itself
  where
    coords = map (\c -> [c, c+cubeDist, c - cubeDist]) cube
    combinations (x:xs) = [y:ys | y <- x, ys <- combinations xs]
    combinations [] = [[]]

-- hash a cube (with coordinates) to a number, to be used as the random seed for that cube
hashCube :: Cube -> Int
hashCube = hash
    
-- generate the random points for a specific cube
sampleCube :: Cube -> [Point]
sampleCube cube = map (\point -> zipWith (\c p -> round ((fromIntegral c) + (p * (fromIntegral cubeDist)))) cube point) points
  where
    rGen = seed [fromIntegral $ hashCube cube]
    pointCount = 4
    dimensions = length cube
    randCount = dimensions * pointCount
    rands = rands' rGen randCount []
    rands' _ 0 l = l
    rands' !g n !l = case range_random (0.0,1.0) g of
       (r,g') -> rands' g' (n-1) (r:l)
    genPts [] acc  = acc
    genPts !rs !acc = case splitAt dimensions rs of
      (coords,rs') -> genPts rs' (coords : acc)
    points :: [[Double]]
    points = genPts rands []

updateCache cube cache = Cache cube cacheVals'
  where 
    cubeSamples = Map.findWithDefault (sampleCube cube) cube (cacheVals cache)
    neighbors = neighborCubes cube -- can be filtered, based on cubeSamples and their distances as well as the position of `point`.
    neighborSamples = map (\c -> (c,sampleCube c)) neighbors
    insertCube m c v = Map.insertWith (\new old -> old) c v m
    cacheVals' = Map.differenceWith (\new old -> Just old) (Map.fromList ((cube,cubeSamples) : neighborSamples)) (cacheVals cache) 
          
-- distances from a point to the 4 next random points
distances :: Cache -> Point -> (((Point,Double),(Point,Double),(Point,Double),(Point,Double)), Cache)
distances cache point = (dists, cache')
  where
    cache' = if cacheCube cache == cube then cache else updateCache cube cache
    dists = fourSmallest (compare `on` snd) $ cubeSamples ++ (map (\p -> (p, distance point p)) (concat neighborSamples))
    cube = pointCube point
    neighbors' = neighborCubes cube -- can be filtered, based on cubeSamples and their distances as well as the position of `point`.
    maxCubeDist = snd $ maximumBy (compare `on` snd) cubeSamples
    neighbors = filter (furtherThan maxCubeDist point) neighbors'
    cubeSamples = map (\p -> (p, distance point p)) (Map.findWithDefault [] cube (cacheVals cache'))
    neighborSamples = map (\c -> Map.findWithDefault [] c (cacheVals cache')) neighbors
    
fourSmallest cmp l = case splitAt 4 l of 
  (f,l') -> case sortBy cmp f of 
    [a,b,c,d] -> fourSmallest' a b c d l'
  where
    fourSmallest' a b c d (x:xs) = 
      if cmp x d == LT then
        if cmp x b == LT then
          if cmp x a == LT then fourSmallest' x a b c xs
          else fourSmallest' a x b c xs
        else
          if cmp x c == LT then fourSmallest' a b x c xs
          else fourSmallest' a b c x xs
      else
        fourSmallest' a b c d xs
    fourSmallest' a b c d [] = (a,b,c,d)
    
-- returns True if a cube is with certainty further away from pt than a certain distance
-- for simplicity, the cube is interpreted as a hypersphere
furtherThan :: Double -> Point -> Cube -> Bool
furtherThan d pt cube = cubeMinDist < d
  where
    cubeCenter = map (+ (cubeDist `div` 2)) cube 
    cubeCenterDist = distance pt cubeCenter
    cubeMinDist = cubeCenterDist - cubeRadius
  
    
-- scales its positive input (which should already be approximately between 0 and 1) to the interval [0, 1)
-- it is a tuned logistic sigmoid function
scalingFunction x = 2 * (1 / (1 + exp ((-1) * (3*x))) - 0.5)

-- alternative scaling functions
-- scalingFunction x = (3*x) / (1+(3*x))

data Val = Val Double
         | Gap Double
           
value :: Val -> Double
value (Val d) = d
value (Gap d) = d
    
-- compute the value (between 0 and 1) for the given point. If the point is between two cells, returns the negative value
valForPx :: Cache -> Point -> (Val, Cache)
valForPx cache pt = val `seq` (val, cache')
  where
    -- (p1,f1'),(p2,f2') and so on are the 4 closest points to `pt` and the distances to them
    -- 
    (((p1,f1'),(p2,f2'),(p3,f3'),(p4,f4')),cache') = distances cache pt
    p1p2Dist    = distance p1 p2
    scaleFactor = p1p2Dist / (distance pt p1 + distance pt p2)
    scaledDist = scalingFunction dist
    [f1,f2,f3,f4] = map (/ distScale) [f1',f2',f3',f4']
    -- Various ways to compute the distance are possible.
    -- All of those listed here result in different more or less interesting effects
    -- dist = f1
    dist = (2 * f1 + f2) / 3
    -- dist = (0.5 * f1 + 0.25 * f2 + f3 / 4) --  + f4 / 12)
    -- dist = (0.5 * f1 + 0.25 * f2 + f3 / 6 + f4 / 12)
    -- dist = f2 - f1 
    -- dist = (2 * f3 - f2 - f1) / 2
    -- The point is between two cells if f2 and f1 are approximately equal.
    -- Simple thresholding of f2 - f1 gives bad results, since the random points are not evenly spaced.
    -- Using the scaleFactor defined above normalizes the point distribution and hence gives us the desired result,
    -- i.e gaps with even width.
    -- Note: there are still some artefacts if p1p2Dist, f1' and f2' are all very small.
    tScaleF = distScale / 40
    val  = if tScaleF * scaleFactor >= f2' - f1'  then Gap scaledDist else Val scaledDist

-- compute the color for a give point
colorForPx :: VoronoiParams -> Cache -> Point -> ([Word8], Cache)
colorForPx params cache pt = case (valForPx cache pt, color3 params) of
  ((Gap val,cache'), Just c3) -> (c3,cache')
  ((val,cache'), _)           -> (greyToColor (color1 params) (color2 params) (value val), cache')

-- size of the image to be generated
width = 2048
height = 2048
    
main = do
  let params = VoronoiParams [60,60,60,255] [255,255,255,255] (Just [0,0,0,255])
  -- let params = VoronoiParams [255,255,255,255] [0,0,0,255] Nothing -- grayscale
  let pixels = [[x,y] | x <- [1..width], y <- [1..height]]
  let (rgbVals,_) = foldl' (\(img,cache) px -> case colorForPx params cache px of (c,cache') -> (c ++ img, cache')) ([],Cache emptyCube Map.empty) pixels
  let rgba = BS.pack rgbVals
  let bmp    = packRGBA32ToBMP width height rgba
  writeBMP "out.bmp" bmp
    