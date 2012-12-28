module Test where
import Data.List

combis [] = [[]]
combis (x:xs) = [y:ys | y <- x, ys <- combis xs]